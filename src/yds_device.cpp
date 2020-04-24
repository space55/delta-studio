#include "../include/yds_device.h"

#include "../include/yds_opengl_device.h"
#include "../include/yds_d3d11_device.h"
#include "../include/yds_d3d10_device.h"

ysDevice::ysDevice() : ysContextObject("API_DEVICE", API_UNKNOWN) {
	m_activeRenderTarget = nullptr;
	m_activeContext = nullptr;
	m_activeVertexBuffer = nullptr;
	m_activeConstantBuffer = nullptr;
	m_activeIndexBuffer = nullptr;
	m_activeShaderProgram = nullptr;
	m_activeInputLayout = nullptr;
	m_activeTextures = nullptr;

	m_maxTextureSlots = 0;
}

ysDevice::ysDevice(DEVICE_API API) : ysContextObject("API_DEVICE", API) {
	m_activeRenderTarget = nullptr;
	m_activeContext = nullptr;
	m_activeVertexBuffer = nullptr;
	m_activeIndexBuffer = nullptr;
	m_activeConstantBuffer = nullptr;
	m_activeShaderProgram = nullptr;
	m_activeInputLayout = nullptr;
	m_activeTextures = nullptr;

	m_maxTextureSlots = 0;
}

ysDevice::~ysDevice() {
    /* void */
}

ysError ysDevice::CreateDevice(ysDevice **newDevice, DEVICE_API API) {
	YDS_ERROR_DECLARE("CreateDevice");

	if (newDevice == nullptr) return YDS_ERROR_RETURN_STATIC(ysError::YDS_INVALID_PARAMETER);
	*newDevice = nullptr;

	if (API == API_UNKNOWN) return YDS_ERROR_RETURN_STATIC(ysError::YDS_INVALID_PARAMETER);

	switch(API) {
	case DIRECTX10:
		*newDevice = new ysD3D10Device;
		break;
	case DIRECTX11:
		*newDevice = new ysD3D11Device;
		break;
	case OPENGL4_0:
		*newDevice = new ysOpenGLDevice;
		break;
	}

    return YDS_ERROR_RETURN_STATIC(ysError::YDS_NO_ERROR);
}

ysError ysDevice::DestroyRenderingContext(ysRenderingContext *&context) {
	YDS_ERROR_DECLARE("DestroyRenderingContext");

	if (context == nullptr) return YDS_ERROR_RETURN(ysError::YDS_INVALID_PARAMETER);

	m_renderingContexts.Delete(context->GetIndex());
	context = nullptr;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::SetContextMode(ysRenderingContext *context, ysRenderingContext::ContextMode mode) {
	YDS_ERROR_DECLARE("SetContextMode");

	if (context == nullptr) return YDS_ERROR_RETURN(ysError::YDS_INVALID_PARAMETER);
	context->m_currentMode = mode;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::SetRenderTarget(ysRenderTarget *newTarget) {
	YDS_ERROR_DECLARE("SetRenderTarget");

	m_activeRenderTarget = newTarget;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysRenderTarget *ysDevice::GetActualRenderTarget() {
	if (m_activeRenderTarget == nullptr) return nullptr;

	if (m_activeRenderTarget->GetType() == ysRenderTarget::Type::Subdivision) {
		return m_activeRenderTarget->GetParent();
	}

	return m_activeRenderTarget;
}

ysError ysDevice::CreateOffScreenRenderTarget(ysRenderTarget **newTarget, const ysRenderTarget *reference) {
	YDS_ERROR_DECLARE("CreateOffScreenRenderTarget");

	if (newTarget == nullptr) return YDS_ERROR_RETURN(ysError::YDS_INVALID_PARAMETER);
	*newTarget = nullptr;

	if (reference == nullptr) return YDS_ERROR_RETURN(ysError::YDS_INVALID_PARAMETER);

	YDS_NESTED_ERROR_CALL( CreateOffScreenRenderTarget(
		newTarget,
		reference->GetWidth(),
		reference->GetHeight(),
		reference->GetFormat(),
		reference->GetSampleCount(),
		reference->HasDepthBuffer()) );

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::ResizeRenderTarget(ysRenderTarget* target, int width, int height) {
    YDS_ERROR_DECLARE("ResizeRenderTarget");

    if (target == nullptr) return YDS_ERROR_RETURN(ysError::YDS_INVALID_PARAMETER);

    if (target->m_associatedContext != nullptr) {
        // This is an on-screen render target so the width and height must match
        // the requirements of the parent context.

        int reqWidth = target->m_associatedContext->GetWindow()->GetScreenWidth();
        int reqHeight = target->m_associatedContext->GetWindow()->GetScreenHeight();

        if (reqWidth != width || reqHeight != height){
            return YDS_ERROR_RETURN_MSG(ysError::YDS_INVALID_PARAMETER, "On-screen render target size must match window size.");
        }
    }

    target->m_width = width;
    target->m_height = height;

    return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::SetDepthTestEnabled(ysRenderTarget *target, bool enable) {
	YDS_ERROR_DECLARE("SetDepthTestEnable");

	target->SetDepthTestEnabled(enable);

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::DestroyRenderTarget(ysRenderTarget *&renderTarget) {
	YDS_ERROR_DECLARE("DestroyRenderTarget");

	if (!renderTarget) return YDS_ERROR_RETURN(ysError::YDS_INVALID_PARAMETER);

	YDS_NESTED_ERROR_CALL( m_renderTargets.Delete(renderTarget->GetIndex()) );
	renderTarget = nullptr;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::UseVertexBuffer(ysGPUBuffer *buffer, int stride, int offset) {
	YDS_ERROR_DECLARE("UseVertexBuffer");

	if (buffer) {
		if (buffer->m_bufferType == ysGPUBuffer::GPU_DATA_BUFFER) {
			m_activeVertexBuffer = buffer;
			m_activeVertexBuffer->m_currentStride = stride;
		}
		else return YDS_ERROR_RETURN(ysError::YDS_INCOMPATIBLE_PLATFORMS);
	}
	else {
		m_activeVertexBuffer = nullptr;
	}

    return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::UseIndexBuffer(ysGPUBuffer *buffer, int offset) {
	YDS_ERROR_DECLARE("UseIndexBuffer");

    if (buffer) {
        if (buffer->m_bufferType == ysGPUBuffer::GPU_INDEX_BUFFER)
            m_activeIndexBuffer = buffer;
        else return YDS_ERROR_RETURN(ysError::YDS_INVALID_GPU_BUFFER_TYPE);
    }
	else {
		m_activeIndexBuffer = nullptr;
	}

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::UseConstantBuffer(ysGPUBuffer *buffer, int slot) {
	YDS_ERROR_DECLARE("UseConstantBuffer");

	if (buffer) {
		if (buffer->m_bufferType == ysGPUBuffer::GPU_CONSTANT_BUFFER) {
			m_activeConstantBuffer = buffer;
		}
		else return YDS_ERROR_RETURN(ysError::YDS_INVALID_GPU_BUFFER_TYPE);
	}
	else {
		m_activeConstantBuffer = nullptr;
	}

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysGPUBuffer *ysDevice::GetActiveBuffer(ysGPUBuffer::GPU_BUFFER_TYPE bufferType) {
	switch(bufferType) {
	case ysGPUBuffer::GPU_CONSTANT_BUFFER:
		return m_activeConstantBuffer;
	case ysGPUBuffer::GPU_DATA_BUFFER:
		return m_activeVertexBuffer;
	case ysGPUBuffer::GPU_INDEX_BUFFER:
		return m_activeIndexBuffer;
	default:
		return nullptr;
	}
}

ysError ysDevice::EditBufferDataRange(ysGPUBuffer *buffer, char *data, int size, int offset) {
	YDS_ERROR_DECLARE("EditBufferDataRange");

	// Error checking
	if (data == nullptr) return YDS_ERROR_RETURN(ysError::YDS_INVALID_PARAMETER);
	if ((size + offset) > buffer->GetSize()) return YDS_ERROR_RETURN(ysError::YDS_OUT_OF_BOUNDS);
	if (size < 0 || offset < 0) return YDS_ERROR_RETURN(ysError::YDS_OUT_OF_BOUNDS);

	if (buffer->m_mirrorToRAM) {
		// Check that the buffer has a RAM buffer
		if (buffer->m_RAMMirror == nullptr) return YDS_ERROR_RETURN(ysError::YDS_UNINTIALIZED_BUFFER);

		// Copy memory
		memcpy(buffer->m_RAMMirror + offset, data, size);
	}

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::EditBufferData(ysGPUBuffer *buffer, char *data) {
	YDS_ERROR_DECLARE("EditBufferData");

	YDS_NESTED_ERROR_CALL( ysDevice::EditBufferDataRange(buffer, data, buffer->GetSize(), 0) );

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::DestroyGPUBuffer(ysGPUBuffer *&buffer) {
	YDS_ERROR_DECLARE("DestroyGPUBuffer");

	if (!buffer) return YDS_ERROR_RETURN(ysError::YDS_INVALID_PARAMETER);

	if (buffer->m_mirrorToRAM) {
		delete [] buffer->m_RAMMirror;
        buffer->m_RAMMirror = nullptr;
	}

	YDS_NESTED_ERROR_CALL( m_gpuBuffers.Delete(buffer->GetIndex()) );
	buffer = nullptr;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR); 
}

ysError ysDevice::DestroyShader(ysShader *&shader) {
	YDS_ERROR_DECLARE("DestroyShader");

	if (shader == nullptr) return YDS_ERROR_RETURN(ysError::YDS_INVALID_PARAMETER);
	if (!CheckCompatibility(shader)) return YDS_ERROR_RETURN(ysError::YDS_INCOMPATIBLE_PLATFORMS);

	YDS_NESTED_ERROR_CALL( m_shaders.Delete(shader->GetIndex()) );
	shader = nullptr;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::DestroyShaderProgram(ysShaderProgram *&program, bool destroyShaders) {
	YDS_ERROR_DECLARE("DestroyShaderProgram");

	if (!CheckCompatibility(program)) return YDS_ERROR_RETURN(ysError::YDS_INCOMPATIBLE_PLATFORMS);

	if (program == m_activeShaderProgram) UseShaderProgram(nullptr);

	if (destroyShaders) {
		for(int i = 0; i < ysShader::SHADER_TYPE_NUM_TYPES; i++) {
			YDS_NESTED_ERROR_CALL( DestroyShader(program->m_shaderSlots[i]) );
		}
	}

	YDS_NESTED_ERROR_CALL( m_shaderPrograms.Delete(program->GetIndex()) );
	program = nullptr;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::AttachShader(ysShaderProgram *program, ysShader *shader) {
	YDS_ERROR_DECLARE("AttachShader");

	if (program == nullptr || shader == nullptr) return YDS_ERROR_RETURN(ysError::YDS_INVALID_PARAMETER);
	if (!CheckCompatibility(program))	return YDS_ERROR_RETURN_MSG(ysError::YDS_INCOMPATIBLE_PLATFORMS, "PROGRAM");
	if (!CheckCompatibility(shader))	return YDS_ERROR_RETURN_MSG(ysError::YDS_INCOMPATIBLE_PLATFORMS, "SHADER");
	if (program->m_isLinked)			return YDS_ERROR_RETURN(ysError::YDS_PROGRAM_ALREADY_LINKED);

	program->m_shaderSlots[shader->m_shaderType] = shader;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::LinkProgram(ysShaderProgram *program) {
	YDS_ERROR_DECLARE("LinkProgram");

	if (program == nullptr)				return YDS_ERROR_RETURN(ysError::YDS_INVALID_PARAMETER);
	if (!CheckCompatibility(program))	return YDS_ERROR_RETURN(ysError::YDS_INCOMPATIBLE_PLATFORMS);
	if (program->m_isLinked)			return YDS_ERROR_RETURN(ysError::YDS_PROGRAM_ALREADY_LINKED);

	program->m_isLinked = true;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::UseShaderProgram(ysShaderProgram *program) {
	YDS_ERROR_DECLARE("UseShaderProgram");

	m_activeShaderProgram = program;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::UseInputLayout(ysInputLayout *layout) {
	YDS_ERROR_DECLARE("UseInputLayout");

	if (!CheckCompatibility(layout)) return YDS_ERROR_RETURN(ysError::YDS_INCOMPATIBLE_PLATFORMS);
	m_activeInputLayout = layout;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::DestroyInputLayout(ysInputLayout *&layout) {
	YDS_ERROR_DECLARE("DestroyInputLayout");

	if (!CheckCompatibility(layout)) return YDS_ERROR_RETURN(ysError::YDS_INCOMPATIBLE_PLATFORMS);

	YDS_NESTED_ERROR_CALL( m_inputLayouts.Delete(layout->GetIndex()) );
	layout = nullptr;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::UseTexture(ysTexture *texture, int slot) {
	YDS_ERROR_DECLARE("UseTexture");

	if (!CheckCompatibility(texture)) return YDS_ERROR_RETURN(ysError::YDS_INCOMPATIBLE_PLATFORMS);
	if (slot < 0 || slot >= this->m_maxTextureSlots) return YDS_ERROR_RETURN(ysError::YDS_OUT_OF_BOUNDS);

	m_activeTextures[slot].Texture = texture;
	m_activeTextures[slot].RenderTarget = nullptr;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::UseRenderTargetAsTexture(ysRenderTarget *texture, int slot) {
	YDS_ERROR_DECLARE("UseTexture");

	if (!CheckCompatibility(texture)) return YDS_ERROR_RETURN(ysError::YDS_INCOMPATIBLE_PLATFORMS);
	if (slot < 0 || slot >= this->m_maxTextureSlots) return YDS_ERROR_RETURN(ysError::YDS_OUT_OF_BOUNDS);

	m_activeTextures[slot].RenderTarget = texture;
	m_activeTextures[slot].Texture = nullptr;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::DestroyTexture(ysTexture *&texture) {
	YDS_ERROR_DECLARE("DestroyTexture");

	if (!CheckCompatibility(texture)) return YDS_ERROR_RETURN(ysError::YDS_INCOMPATIBLE_PLATFORMS);

	YDS_NESTED_ERROR_CALL( m_textures.Delete(texture->GetIndex()) );
	texture = nullptr;

	return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}

ysError ysDevice::InitializeTextureSlots(int maxSlots) {
	YDS_ERROR_DECLARE("InitializeTextureSlots");

	m_activeTextures = new ysTextureSlot [maxSlots];
	memset(m_activeTextures, 0, sizeof(ysTextureSlot *) * maxSlots);

	m_maxTextureSlots = maxSlots;

    return YDS_ERROR_RETURN(ysError::YDS_NO_ERROR);
}