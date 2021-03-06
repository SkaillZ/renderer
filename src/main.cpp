#include <stdexcept>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Renderer.hpp"
#include "Light.hpp"
#include "Globals.hpp"
#include "ModelLoader.hpp"
#include "Window.hpp"
#include "Splines.hpp"
#include "KdTree.hpp"

const std::string MECH_PATH = "models/model.dae";
const std::string CUBE_PATH = "models/cube.obj";
const std::string SPHERE_PATH = "models/sphere.obj";
const std::string TERRAIN_PATH = "models/terrain.obj";

const std::string TEXTURE_PATH = "textures/default_albedo.jpg";
const std::string MASK_TEXTURE_PATH = "textures/default_mgao.jpg";
const std::string NORMAL_MAP_PATH = "textures/default_normal.jpg";

const std::string GROUND_TEXTURE_PATH = "textures/terrain_albedo.jpeg";
const std::string GROUND_MASK_TEXTURE_PATH = "textures/terrain_mgao.png";
const std::string GROUND_NORMAL_MAP_PATH = "textures/terrain_normal.jpeg";

const float CAMERA_MOVE_SPEED = 3.0f;
const float CAMERA_ROTATE_SPEED = 0.03f;
const float CAMERA_TRACKING_SPEED_INCREASE = 0.5f;
const float CAMERA_TRACKING_STEP_SIZE = 0.0004f;

struct CameraWaypoint {
    glm::vec3 position;
    glm::quat rotation;

    CameraWaypoint(): position(), rotation(glm::identity<glm::quat>()) { }
    CameraWaypoint(glm::vec3 position, glm::quat rotation): position(position), rotation(rotation) { }
};

static auto window = std::make_shared<Window>(WIDTH, HEIGHT);
static int maxMsaaSamples = 64;
static bool rayLocked = false;

void framebufferResizeCallback(GLFWwindow* nativeWindow, int width, int height) {
    auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(nativeWindow));
    renderer->setFramebufferResized();
}

void keyPressedCallback(GLFWwindow* nativeWindow, int key, int scancode, int action, int mods) {
    auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(nativeWindow));

    if (action != GLFW_RELEASE)
        return;

    if (key == GLFW_KEY_ESCAPE) {
		window->setMouseLocked(!window->isMouseLocked());
    } else if (key == GLFW_KEY_F11) {
        if (window->isFullscreen()) {
            window->exitFullscreen();
        }
        else {
            window->enterFullscreen();
        }
    } else if (key == GLFW_KEY_M) {
        auto samples = glm::clamp(renderer->getDevice().getUserRequestedMsaaSamples() * 2, 1, maxMsaaSamples);
        renderer->getDevice().setUserRequestedMsaaSamples(samples);
        std::cout << "Set MSAA sample count to " << samples << std::endl;
        renderer->recreateSwapChain();
    } else if (key == GLFW_KEY_N) {
        auto samples = glm::clamp(renderer->getDevice().getUserRequestedMsaaSamples() / 2, 1, maxMsaaSamples);
        renderer->getDevice().setUserRequestedMsaaSamples(samples);
        std::cout << "Set MSAA sample count to " << samples << std::endl;
        renderer->recreateSwapChain();
    }
    else if (key == GLFW_KEY_L) {
        rayLocked = !rayLocked;
    }
}

int modulo(int a, int b) {
    /** Modulo with correct handling for a negative a */
    int result = a % b;
    return result >= 0 ? result : result + b;
}

CameraWaypoint getInterpolatedWaypoint(std::vector<CameraWaypoint>& waypoints, float t) {
    /** Calculates an interpolated waypoint at the normalized time t */
    int waypointNum = waypoints.size();
    int waypointIndex = static_cast<int>(t * waypointNum);

    // Get the four points we need for interpolation (wp2 is the next point)
    auto& wp0 = waypoints[modulo(waypointIndex - 2, waypointNum)];
    auto& wp1 = waypoints[modulo(waypointIndex - 1, waypointNum)];
    auto& wp2 = waypoints[waypointIndex];
    auto& wp3 = waypoints[(waypointIndex + 1) % waypointNum];

    float positionInWaypoint = fmod(t * waypoints.size(), 1);
    auto position = Splines::catmullRom(wp0.position, wp1.position, wp2.position, wp3.position, positionInWaypoint);

    auto intermediate1 = glm::intermediate(wp0.rotation, wp1.rotation, wp2.rotation);
    auto intermediate2 = glm::intermediate(wp1.rotation, wp2.rotation, wp3.rotation);
    auto rotation = glm::squad(wp1.rotation, wp2.rotation, intermediate1, intermediate2, positionInWaypoint);

    return CameraWaypoint(position, rotation);
}

CameraWaypoint getInterpolatedWaypointAtDistance(std::vector<CameraWaypoint>& waypoints, float distance) {
    /** Calculates an interpolated waypoint based on the given distance. */
    float t = 0.0f;
    float currentDistance = 0.0f;
    auto lastWaypoint = getInterpolatedWaypoint(waypoints, 0.0f);

    while (currentDistance < distance) {
        auto waypoint = getInterpolatedWaypoint(waypoints, t);
        currentDistance += glm::distance(lastWaypoint.position, waypoint.position);
        lastWaypoint = waypoint;
        t += CAMERA_TRACKING_STEP_SIZE;

        if (t >= 1.0f) {
            // Keep showing the last waypoint if the distance is out of bounds
            t = 1.0f;
            break;
        }
    };

    return lastWaypoint;
}

int main() {
    glfwSetFramebufferSizeCallback(window->getNativeWindowPointer(), framebufferResizeCallback);
    
    auto renderer = std::make_unique<Renderer>(window->getNativeWindowPointer());
    glfwSetWindowUserPointer(window->getNativeWindowPointer(), renderer.get());
    maxMsaaSamples = renderer->getDevice().maxMsaaSamples;
    
    auto colorTexture = std::make_shared<VulkanTexture>(TEXTURE_PATH, renderer->getDevice(), true);
    auto maskTexture = std::make_shared<VulkanTexture>(MASK_TEXTURE_PATH, renderer->getDevice(), false);
    auto normalMapTexture = std::make_shared<VulkanTexture>(NORMAL_MAP_PATH, renderer->getDevice(), false);
    
    auto groundColorTexture = std::make_shared<VulkanTexture>(GROUND_TEXTURE_PATH, renderer->getDevice(), true);
    auto groundMaskTexture = std::make_shared<VulkanTexture>(GROUND_MASK_TEXTURE_PATH, renderer->getDevice(), false);
    auto groundNormalMapTexture = std::make_shared<VulkanTexture>(GROUND_NORMAL_MAP_PATH, renderer->getDevice(), false);
    
    auto skyboxTexture = VulkanTexture::loadCubemap({
        "textures/right.jpg",
        "textures/left.jpg",
        "textures/top.jpg",
        "textures/bottom.jpg",
        "textures/front.jpg",
        "textures/back.jpg"
    }, renderer->getDevice(), true);
    
    auto characterUniforms = std::make_shared<Uniforms<LocalTransform>>(renderer->getDevice(), 3, true);
    auto skyboxUniforms = std::make_shared<Uniforms<LocalTransform>>(renderer->getDevice(), 1, false);
    auto groundUniforms = std::make_shared<Uniforms<LocalTransform>>(renderer->getDevice(), 3, true);
    auto kdTreeUniforms = std::make_shared<Uniforms<LocalTransform>>(renderer->getDevice(), 0, true);
    auto hitTriangleUniforms = std::make_shared<Uniforms<LocalTransform>>(renderer->getDevice(), 0, true);
    auto hitIndicatorUniforms = std::make_shared<Uniforms<LocalTransform>>(renderer->getDevice(), 0, true);
    
    auto staticPipeline = PipelineSettingsBuilder()
        .vertexShader("shaders/static.vert.spv")
        .shadowVertexShader("shaders/shadowpass.vert.spv")
        .fragmentShader("shaders/shader.frag.spv")
        .build();

    auto linesPipeline = PipelineSettingsBuilder()
        .vertexShader("shaders/static.vert.spv")
        .fragmentShader("shaders/bluesolid.frag.spv")
        .topology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
        .build();

    auto hitTrianglePipeline = PipelineSettingsBuilder()
        .vertexShader("shaders/static.vert.spv")
        .fragmentShader("shaders/redsolid.frag.spv")
        .depthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL) // Draw the hit triangle on top
        .build();

    auto hitIndicatorPipeline = PipelineSettingsBuilder()
        .vertexShader("shaders/static.vert.spv")
        .fragmentShader("shaders/greensolid.frag.spv")
        .build();
    
    auto skyboxPipeline = PipelineSettingsBuilder()
        .vertexShader("shaders/skybox.vert.spv")
        .fragmentShader("shaders/skybox.frag.spv")
        .depthWrite(false)
        .depthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
        .cullMode(VK_CULL_MODE_FRONT_BIT)
        .build();
    
    auto character = ModelLoader::fromFile(MECH_PATH, renderer->getDevice(), staticPipeline, std::move(characterUniforms));
    auto skybox = ModelLoader::fromFile(CUBE_PATH, renderer->getDevice(), skyboxPipeline, std::move(skyboxUniforms));
    auto ground = ModelLoader::fromFile(TERRAIN_PATH, renderer->getDevice(), staticPipeline, std::move(groundUniforms));
    auto hitIndicator = ModelLoader::fromFile(SPHERE_PATH, renderer->getDevice(), hitIndicatorPipeline, std::move(hitIndicatorUniforms));
    
    character->getUniforms().addTexture(2, std::move(colorTexture));
    character->getUniforms().addTexture(3, std::move(maskTexture));
    character->getUniforms().addTexture(4, std::move(normalMapTexture));
    
    colorTexture = std::make_shared<VulkanTexture>(TEXTURE_PATH, renderer->getDevice(), true);
    maskTexture = std::make_shared<VulkanTexture>(MASK_TEXTURE_PATH, renderer->getDevice(), false);
    normalMapTexture = std::make_shared<VulkanTexture>(NORMAL_MAP_PATH, renderer->getDevice(), false);
    
    ground->getUniforms().addTexture(2, std::move(groundColorTexture));
    ground->getUniforms().addTexture(3, std::move(groundMaskTexture));
    ground->getUniforms().addTexture(4, std::move(groundNormalMapTexture));

    skybox->getUniforms().addTexture(2, std::move(skyboxTexture));

    character->position = glm::vec3(0.0f, 0.5f, 0.0f);

    skybox->scale = glm::vec3(10.0f, 10.0f, 10.0f);

    ground->position = glm::vec3(0.0f, -0.05f, 0.0f);
    ground->scale = glm::vec3(15.0f, 15.0f, 15.0f);

    std::cout << "Building k-d tree..." << std::endl;
    auto kdTree = std::make_shared<KdTree>(std::vector<std::shared_ptr<Model>> { character, ground });
    std::cout << "k-d tree build finished. Creating visual model..." << std::endl;

    auto kdTreeModel = kdTree->createLineModelForBoundingBoxes(renderer->getDevice(), linesPipeline, std::move(kdTreeUniforms));
    auto kdTreeTriModel = kdTree->createHitTriangleModel(renderer->getDevice(), hitTrianglePipeline, std::move(hitTriangleUniforms));
    std::cout << "Creating visual model finished." << std::endl;

    renderer->addModel(character);
    renderer->addModel(ground);
    renderer->addModel(skybox);
    renderer->addModel(kdTreeModel);
    renderer->addModel(kdTreeTriModel);
    renderer->addModel(hitIndicator);

    auto lightPos = glm::vec3(0, 5, 10);
    auto lightDir = glm::vec3(0.0f, 0.5f, 0.8f);
    renderer->setLight(0, Light::createDirectionalLight(lightDir, glm::vec3(5.0f, 5.0f, 5.0f)));
    renderer->setAmbientLight(glm::vec3(0.1f, 0.1f, 0.1f));
    
    renderer->finishInitialization();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    float lastTime = 0.0f;

    auto& cam = renderer->getCamera();
    cam.position = glm::vec3(0.0f, 0.0f, 5.0f);
    cam.rotation = glm::angleAxis(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    cam.fovy = 45.0f;
    cam.nearPlane = 0.1f;
    cam.farPlane = 100.0f;

    window->setMouseLocked(true);
    glfwSetKeyCallback(window->getNativeWindowPointer(), keyPressedCallback);

    // Camera movement setup
    std::vector<CameraWaypoint> waypoints = {};
    glm::vec2 lastMousePos = window->getMousePosition();
    bool cameraTrackingActive = false;
    float currentCameraDistance = 0.0f;
    float cameraTrackingSpeed = 2.5f;

    static float normalIntensity = 1.0f;
    
    try {
        while (!window->shouldClose()) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
            float deltaTime = time - lastTime;
            lastTime = time;

            glfwPollEvents();
            auto mousePos = window->getMousePosition();
            auto mouseDelta = (lastMousePos - mousePos) * deltaTime;
            lastMousePos = mousePos;

            cam.aspectRatio = renderer->getExtent().x / static_cast<float>(renderer->getExtent().y);
            
            if (window->isMouseLocked()) {
                // The first rotation is in world space, so multiply the current rotation with the yaw
                cam.rotation *= glm::angleAxis(mouseDelta.x * CAMERA_ROTATE_SPEED, glm::vec3(0.0f, 1.0f, 0.0f));
                // The second rotation should be in local space for the right behaviour, so multiply the pitch the other way around
                cam.rotation = glm::angleAxis(mouseDelta.y * CAMERA_ROTATE_SPEED, glm::vec3(1.0f, 0.0f, 0.0f)) * cam.rotation;
            }

            float forward = 0.0f;
            float right = 0.0f;

            if (window->getKey(GLFW_KEY_UP) || window->getKey(GLFW_KEY_W)) {
                forward -= 1;
            }
            if (window->getKey(GLFW_KEY_DOWN) || window->getKey(GLFW_KEY_S)) {
                forward += 1;
            }
            if (window->getKey(GLFW_KEY_RIGHT) || window->getKey(GLFW_KEY_D)) {
                right -= 1;
            }
            if (window->getKey(GLFW_KEY_LEFT) || window->getKey(GLFW_KEY_A)) {
                right += 1;
            }
            if (window->getKey(GLFW_KEY_LEFT) || window->getKey(GLFW_KEY_A)) {
                right += 1;
            }
            if (window->getKey(GLFW_KEY_X)) {
                normalIntensity -= 5.f * deltaTime;
                std::cout << "NormalIntensity: " << normalIntensity << std::endl;
            }
            if (window->getKey(GLFW_KEY_C)) {
                normalIntensity += 5.f * deltaTime;
                std::cout << "NormalIntensity: " << normalIntensity << std::endl;
            }

            normalIntensity = glm::clamp(normalIntensity, 0.01f, 15.0f);
            renderer->getGlobals().normalIntensity = normalIntensity;

            cam.position += glm::rotate(glm::inverse(cam.rotation), glm::vec3(0.0f, 0.0f, forward * CAMERA_MOVE_SPEED)) * deltaTime;
            cam.position += glm::rotate(glm::inverse(cam.rotation), glm::vec3(right * CAMERA_MOVE_SPEED, 0.0f, 0.0f)) * deltaTime;

            if (window->getKey(GLFW_KEY_SPACE) && !cameraTrackingActive) {
                // Avoid duplicate waypoints
                if (waypoints.size() == 0 || glm::distance2(cam.position, waypoints.back().position) > 0.5f * 0.5f) {
                    waypoints.push_back(CameraWaypoint(cam.position, cam.rotation));
                    std::cout << "Added waypoint #" << waypoints.size() << " at ("
                        << cam.position.x << ", " << cam.position.y << ", " << cam.position.z << ")" << std::endl;

                    if (waypoints.size() == 20) {
                        cameraTrackingActive = true;
                    }
                }
            }

            if (cameraTrackingActive) {
                // Change speed interactively
                if (window->getKey(GLFW_KEY_Q)) {
                    cameraTrackingSpeed -= CAMERA_TRACKING_SPEED_INCREASE * deltaTime;
                    std::cout << "Tracking speed: " << cameraTrackingSpeed << std::endl;
                }
                if (window->getKey(GLFW_KEY_E)) {
                    cameraTrackingSpeed += CAMERA_TRACKING_SPEED_INCREASE * deltaTime;
                    std::cout << "Tracking speed: " << cameraTrackingSpeed << std::endl;
                }
                cameraTrackingSpeed = glm::clamp(cameraTrackingSpeed, 0.5f, 5.0f);

                // Integrate camera speed and add to current distance
                currentCameraDistance += deltaTime * cameraTrackingSpeed;

                // Interpolation at current distance
                auto waypoint = getInterpolatedWaypointAtDistance(waypoints, currentCameraDistance);
                
                cam.position = waypoint.position;
                cam.rotation = waypoint.rotation;
            }

            auto& characterUniforms = character->getUniforms();
            auto& groundUniforms = ground->getUniforms();
            
            // -- Shadow map uniforms --
            // Keep depth range as small as possible for better shadow map precision
            float zNear = 0.1f;
            float zFar = 20.0f;
            glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, zNear, zFar);
            glm::mat4 depthViewMatrix = glm::lookAt(lightPos, lightPos - lightDir, glm::vec3(0, 1, 0));
            
            characterUniforms.ubo.lightSpace = depthProjectionMatrix * depthViewMatrix;
            groundUniforms.ubo.lightSpace = depthProjectionMatrix * depthViewMatrix;

            // KdTree raycast
            glm::vec3 direction = glm::vec3(0.0f, 0.0f, 1.0f) * cam.rotation;
            glm::vec3 origin = -cam.position;
            float maxDistance = 10.0f;

            auto startTime = std::chrono::high_resolution_clock::now();

            auto hit = kdTree->raycast(origin, direction, maxDistance);

            auto endTime = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

            if (!rayLocked) {
                std::cout << "Raycast took " << ms << " microseconds. Hit distance: " << hit.distance << std::endl;

                auto triMesh = kdTreeTriModel->getMeshes()[0];

                triMesh->vertices[0].pos = hit.triangle[0];
                triMesh->vertices[1].pos = hit.triangle[1];
                triMesh->vertices[2].pos = hit.triangle[2];

                triMesh->updateVertexBuffer();

                hitIndicator->position = hit.distance != INFINITY ? hit.point : origin + direction * maxDistance;
            }

            renderer->drawFrame();
        }
        renderer->waitForDeviceIdle();
        
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    ground.reset();
    skybox.reset();
    character.reset();
    
    colorTexture.reset();
    maskTexture.reset();
    normalMapTexture.reset();
    groundColorTexture.reset();
    groundMaskTexture.reset();
    groundNormalMapTexture.reset();

    renderer.reset();
    
	window.reset();
    
    return EXIT_SUCCESS;
}
