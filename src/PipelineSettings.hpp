#pragma once

#include <vulkan/vulkan.h>

struct PipelineSettings {
    std::string vertexShader;
    std::string shadowVertexShader;
    std::string fragmentShader;
    bool depthTest;
    bool depthWrite;
    VkCullModeFlags cullMode;
    VkCompareOp depthCompareOp;
};

class PipelineSettingsBuilder {
    
public:
    PipelineSettingsBuilder& vertexShader(std::string path) {
        m_vertexShader = path;
        return *this;
    }
    
    PipelineSettingsBuilder& shadowVertexShader(std::string path) {
        m_shadowVertexShader = path;
        return *this;
    }
    
    PipelineSettingsBuilder& fragmentShader(std::string path) {
        m_fragmentShader = path;
        return *this;
    }
    
    PipelineSettingsBuilder& depthTest(bool value) {
        m_depthTest = value;
        return *this;
    }
    
    PipelineSettingsBuilder& depthWrite(bool value) {
        m_depthWrite = value;
        return *this;
    }
    
    PipelineSettingsBuilder& cullMode(VkCullModeFlags cullMode) {
        m_cullMode = cullMode;
        return *this;
    }
    
    PipelineSettingsBuilder& depthCompareOp(VkCompareOp depthCompareOp) {
        m_depthCompareOp = depthCompareOp;
        return *this;
    }
    
    std::shared_ptr<PipelineSettings> build() {
        auto settings = std::make_shared<PipelineSettings>();
        settings->vertexShader = m_vertexShader;
        settings->shadowVertexShader = m_shadowVertexShader;
        settings->fragmentShader = m_fragmentShader;
        settings->depthTest = m_depthTest;
        settings->depthWrite = m_depthWrite;
        settings->cullMode = m_cullMode;
        settings->depthCompareOp = m_depthCompareOp;
        return settings;
    }
    
private:
    std::string m_vertexShader;
    std::string m_shadowVertexShader = "";
    std::string m_fragmentShader;
    bool m_depthTest = true;
    bool m_depthWrite = true;
    VkCullModeFlags m_cullMode = VK_CULL_MODE_BACK_BIT;
    VkCompareOp m_depthCompareOp = VK_COMPARE_OP_LESS;
    
};
