//
// Created by wangyuelin on 2020/10/27.
//

#include "GLRender.h"

void GLRender::Release() {

}

int GLRender::Init() {
    int ret = 0;
    if (m_eInitStatus == SUCCESS) {
        LOGE(TAG, "GL已经初始化, 不用再吃初始化");
        return RET_SUCCESS;
    }
   ret = CreateProgram();
    if (!ret) {
        LOGE(TAG, "创建GL程序失败");
        return RET_ERROR;
    }
    LOGE(TAG, "创建GL程序成功");

    return RET_SUCCESS;
}

GLint GLRender::__LoadShader(GLenum type, const GLchar *pShaderCode) {
    //据type创建定点着色器或者片元着色器
    GLint shader = glCreateShader(type);
    //加载着色器代码
    glShaderSource(shader, 1, &pShaderCode, NULL);
    //编译代码
    glCompileShader(shader);

    //检查编译的状态
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        //编译失败，输出编译的信息
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 0) {
            //申请存日志的内存
            GLchar *pLog = static_cast<GLchar *>(malloc(sizeof(GLchar) * infoLen));
            glGetShaderInfoLog(shader, infoLen, NULL, pLog);
            LOGE(TAG, "创建着色器并编译代码失败：%s", pLog);
            free(pLog);
        }
        glDeleteShader(shader);
        return RET_ERROR;
    }
    return shader;
}


int GLRender::CreateProgram() {
    if (m_nProgramId == 0) {
        //创建一个空的程序对象，这个对象能被着色器shader附着(attached)
        m_nProgramId = glCreateProgram();
        if (m_nProgramId == 0 || glGetError() != GL_NO_ERROR) {
            LOGE(TAG, "glCreateProgram 错误：%x", glGetError());
            return RET_ERROR;
        }
        //加载顶点着色器
        GLint vertexShader = __LoadShader(GL_VERTEX_SHADER, m_pVertexCode);
        if (!vertexShader) {
            LOGE(TAG, "创建顶点着色器失败");
            return RET_ERROR;
        }
        //将其加入到程序
        glAttachShader(m_nProgramId, vertexShader);
        //加载片元着色器
        GLint fragmentShader = __LoadShader(GL_FRAGMENT_SHADER, m_pTextureCode);
        if (!fragmentShader) {
            LOGE(TAG, "创建片元着色器失败");
            return RET_ERROR;
        }
        //将其加入到程序中
        glAttachShader(m_nProgramId, fragmentShader);
        //链接程序
        glLinkProgram(m_nProgramId);
        //检查连接的状态
        GLint linked;
        glGetProgramiv(m_nProgramId, GL_LINK_STATUS, &linked);
        if (!linked) {
            GLint infoLen;
            glGetProgramiv(m_nProgramId, GL_INFO_LOG_LENGTH, &infoLen);
            LOGE(TAG, "Error linking length: %d", infoLen);

            if(infoLen > 1){
                char* infoLog = (char*)malloc(infoLen);
                glGetProgramInfoLog(m_nProgramId, infoLen, NULL, infoLog);
                LOGE("Error linking program: %s, %s", infoLog, glGetString(glGetError()));
                free(infoLog);
            }
            glDeleteProgram(m_nProgramId);
        }
        //获取编译好的着色器中的指定变量的位置
        m_nVertexCoordLoc = glGetAttribLocation(m_nProgramId, "aCoordinate");
        m_nVertexPosLoc = glGetAttribLocation(m_nProgramId, "aPosition");
        m_nTextureMatrixLoc = glGetUniformLocation(m_nProgramId, "uTexture");

    }
    //程序创建成功
    if (m_nProgramId != 0) {
        //将程序对象安装为当前的渲染状态的一部分
        glUseProgram(m_nProgramId);
        LOGE(TAG, "着色器程序创建成功");
    }
    return RET_SUCCESS;
}

int GLRender::CreateAndBindTexture() {
    //创建纹理
    if (m_nTextureId == 0) {
        glGenTextures(1, &m_nTextureId);
        LOGI(TAG, "Create texture id : %d, %x", m_nTextureId, glGetError())
    }
    if (m_nTextureId == 0) {
        LOGE(TAG, "纹理id为空");
        return RET_ERROR;
    }
    //激活指定纹理单元
    glActiveTexture(GL_TEXTURE0);
    checkGlError("glActiveTexture");
    //绑定纹理单元到纹理id
    glBindTexture(GL_TEXTURE_2D, m_nTextureId);
    checkGlError("glBindTexture");
    glUniform1i(m_nTextureMatrixLoc ,0);
    checkGlError("glUniform1i");
    //配置边缘过度参数
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    checkGlError("glTexParameterf");
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    checkGlError("glTexParameterf");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    checkGlError("glTexParameterf");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    checkGlError("glTexParameterf");

    LOGE(TAG, "纹理绑定成功");
    return RET_SUCCESS;
}

void GLRender::checkGlError(const char *op) {
    for (GLint error = glGetError(); error; error = glGetError()) {
        LOGI(TAG, "after %s() glError (0x%x)\n", op, error);
    }

}

int GLRender::BindBitmap(void *pixels, int w, int h) {
    if (pixels == nullptr) {
        LOGE(TAG, "Bitmap像素数据为空");
        return RET_ERROR;
    }
    //使用图像更新一个2d纹理
    glTexImage2D(GL_TEXTURE_2D, 0, // level一般为0
                 GL_RGBA, //纹理内部格式
                 w, h, // 画面宽高
                 0, // 必须为0
                 GL_RGBA, // 数据格式，必须和上面的纹理格式保持一直
                 GL_UNSIGNED_BYTE, // RGBA每位数据的字节数，这里是BYTE​: 1 byte
                 pixels);// 画面数据
    return RET_SUCCESS;
}

void GLRender::Draw() {
    //启用顶点的句柄
    glEnableVertexAttribArray(m_nVertexPosLoc);
    glEnableVertexAttribArray(m_nVertexCoordLoc);
    //设置着色器参数
    glVertexAttribPointer(m_nVertexPosLoc, 2, GL_FLOAT, GL_FALSE, 0, m_gVertexCoors);
    glVertexAttribPointer(m_nVertexCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, m_gTextureCoors);
    //开始绘制
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


