//
// Created by wangyuelin on 2020/11/11.
//

#include "Ogl.h"


#define GET_STR(x) #x /* #x 直接把传入的 X 转换为字符串 直接参考 ijkplayer*/
static const char *vertexShaderCode = GET_STR(
        attribute
        vec4 aPosition; //顶点坐标
        attribute
        vec2 aTexCoord; //材质顶点坐标
        varying
        vec2 vTexCoord;   //输出的材质坐标
        void main() {
            vTexCoord = aTexCoord;
            gl_Position = aPosition;
        }
);

static const char *fragShaderCode = GET_STR(
        precision
        mediump float;    //精度
        varying
        vec2 vTexCoord;     //顶点着色器传递的坐标
        uniform
        sampler2D uTexture;  //输入的材质（不透明灰度，单像素）

        void main() {
            //输出像素颜色
            gl_FragColor = texture2D(uTexture, vTexCoord);
        }
);


bool Opengl::CreateProgram() {
    if (!linkShader()) {
        LOGE(TAG, "着色器链接失败");
        return false;
    }
    if (!PrepareTexture()) {
        LOGE(TAG, "纹理单元准备失败");
        return false;
    }
    return true;


}

GLuint Opengl::loadShader(const char *code, GLint type) {
    //1. 创建shader
    GLuint sh = glCreateShader(type);
    if (sh == 0) {
        LOGE(TAG, "glCreateShader %d failed!", type);
        return 0;
    }
    //2. 加载shader
    glShaderSource(sh,
                   1,    //shader数量
                   &code, //shader代码
                   NULL);   //代码长度
    //3. 编译shader
    glCompileShader(sh);

    //4. 获取编译情况
    GLint status;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        LOGE(TAG, "glCompileShader failed!");
        glDeleteShader(sh);
        return 0;
    }
    LOGE(TAG, "glCompileShader success!");
    return sh;
}

bool Opengl::linkShader() {
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint program;
    GLint linked;

    //加载着色器
    vertexShader = loadShader(vertexShaderCode, GL_VERTEX_SHADER);
    if (vertexShader == 0) {
        LOGE(TAG, "加载顶点着色器失败");
        return false;
    }
    fragmentShader = loadShader(fragShaderCode, GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        LOGE(TAG, "加载片元着色器失败");
        return false;
    }

    // Create the program object
    program = glCreateProgram();
    if (program == 0) {
        LOGE(TAG, "glCreateProgram 失败")
        return false;
    }
    // Attaches a shader object to a program object
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    // Bind vPosition to attribute 0
//    glBindAttribLocation(program, 0, "vPosition");
    // Link the program object
    glLinkProgram(program);

    // Check the link status
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1) {
            GLchar *infoLog = (GLchar *) malloc(sizeof(GLchar) * infoLen);

            glGetProgramInfoLog(program, infoLen, NULL, infoLog);
            LOGE(TAG, "Error linking program:%s", infoLog);
            free(infoLog);
        }
        glDeleteProgram(program);
        return false;
    }

    // Free no longer needed shader resources
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glUseProgram(program);
    this->program = program;
    LOGE(TAG, "着色器链接成功");

    //设置属性
    //设置顶点数据
    static float vertexs[] = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            -1.0f, 1.0f,
            1.0f, 1.0f
    };
    GLuint aPosition = (GLuint) glGetAttribLocation(program, "aPosition");//获取 shader 顶点坐标
    glEnableVertexAttribArray(aPosition);
    //传递顶点
    glVertexAttribPointer(aPosition, 2/*x,y*/, GL_FLOAT, GL_FALSE, 8/*4byte * 2*/, vertexs);

    //设置纹理坐标
    static float textureCoors[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f
    };
    GLuint aTexCoord = (GLuint) glGetAttribLocation(program, "aTexCoord");
    glEnableVertexAttribArray(aTexCoord);
    glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, 8, textureCoors);

    // Tell the texture uniform sampler to use this texture in the shader by
    // telling it to read from texture unit 0.
    //因为前面选择的纹理单元就是0
    glUniform1i(glGetUniformLocation(program, "uTexture"), 0);
    LOGE(TAG, "着色器属性设置成功");
    return true;
}

bool Opengl::PrepareTexture() {
    glGenTextures(1, &textureID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);
    return true;
}


void Opengl::draw(void *pixels, int width, int height, EGLDisplay dpy, EGLSurface surface) {
    glTexImage2D(GL_TEXTURE_2D,
                 0,           //细节基本 0默认
                 GL_RGBA,//gpu内部格式 亮度，灰度图
                 width, height, //拉升到全屏
                 0,             //边框
                 GL_RGBA,//数据的像素格式 亮度，灰度图 要与上面一致
                 GL_UNSIGNED_BYTE, //像素的数据类型
                 pixels              //纹理的数据
    );

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    eglSwapBuffers(dpy, surface);
}
