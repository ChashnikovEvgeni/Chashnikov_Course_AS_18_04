//internal includes
#include "common.h"
#include "ShaderProgram.h"
#include "Camera.h"
#define PI 3.14159265 
//External dependencies
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <random>
#include <map>
// Include GLM
#include <C:\Camp\OpenGL\2021-opengl\glm-0.9.7.1\glm\glm.hpp>
#include <C:\Camp\OpenGL\2021-opengl\glm-0.9.7.1\glm\gtc\matrix_transform.hpp>
using namespace glm;
using namespace std;


static const GLsizei WIDTH = 1080, HEIGHT = 720; //размеры окна
static int filling = 0;
static bool keys[1024]; //массив состояний кнопок - нажата/не нажата
static GLfloat lastX = 400, lastY = 300; //исходное положение мыши
static bool firstMouse = true;
static bool g_captureMouse = true;  // Мышка захвачена нашим приложением или нет?
static bool g_capturedMouseJustNow = false;
static int g_shaderProgram = 0;


GLfloat deltaTime = 0.0f;  // что это? что-то дефолтное
GLfloat lastFrame = 0.0f;

Camera camera(float3(1.0f, 10.0f, 30.0f));  // исходные положение камеры
GLuint loadBMP_custom(const char* imagepath) {

    printf("Reading image %s\n", imagepath);

    // Data read from the header of the BMP file
    unsigned char header[54];
    unsigned int dataPos;
    unsigned int imageSize;
    unsigned int width, height;
    // Actual RGB data
    unsigned char* data;

    // Open the file
    FILE* file = fopen(imagepath, "rb");
    if (!file) {
        printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath);
        getchar();
        return 0;
    }

    // Read the header, i.e. the 54 first bytes

    // If less than 54 bytes are read, problem
    if (fread(header, 1, 54, file) != 54) {
        printf("Not a correct BMP file\n");
        fclose(file);
        return 0;
    }
    // A BMP files always begins with "BM"
    if (header[0] != 'B' || header[1] != 'M') {
        printf("Not a correct BMP file\n");
        fclose(file);
        return 0;
    }
    // Make sure this is a 24bpp file
    if (*(int*)&(header[0x1E]) != 0) { printf("Not a correct BMP file\n");    fclose(file); return 0; }
    if (*(int*)&(header[0x1C]) != 24) { printf("Not a correct BMP file\n");    fclose(file); return 0; }

    // Read the information about the image
    dataPos = *(int*)&(header[0x0A]);
    imageSize = *(int*)&(header[0x22]);
    width = *(int*)&(header[0x12]);
    height = *(int*)&(header[0x16]);

    // Some BMP files are misformatted, guess missing information
    if (imageSize == 0)    imageSize = width * height * 3; // 3 : one byte for each Red, Green and Blue component
    if (dataPos == 0)      dataPos = 54; // The BMP header is done that way

    // Create a buffer
    data = new unsigned char[imageSize];

    // Read the actual data from the file into the buffer
    fread(data, 1, imageSize, file);

    // Everything is in memory now, the file can be closed.
    fclose(file);

    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

    // OpenGL has now copied the data. Free our own version
    delete[] data;

    // Poor filtering, or ...
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 

    // ... nice trilinear filtering ...
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // ... which requires mipmaps. Generate them automatically.
    glGenerateMipmap(GL_TEXTURE_2D);

    // Return the ID of the texture we just created
    return textureID;
}
//функция для обработки нажатий на кнопки клавиатуры
void OnKeyboardPressed(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    switch (key)
    {
    case GLFW_KEY_ESCAPE: //на Esc выходим из программы
        if (action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);
        break;
    case GLFW_KEY_SPACE: //на пробел переключение в каркасный режим и обратно
        if (action == GLFW_PRESS)
        {
            if (filling == 0)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                filling = 1;
            }
            else
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                filling = 0;
            }
        }
        break;
    case GLFW_KEY_1:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        break;
    case GLFW_KEY_2:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        break;
    default:
        if (action == GLFW_PRESS)
            keys[key] = true;   // это чё?
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}

//функция для обработки клавиш мыши
void OnMouseButtonClicked(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        g_captureMouse = !g_captureMouse;


    if (g_captureMouse)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        g_capturedMouseJustNow = true;
    }
    else
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

//функция для обработки перемещения мыши
void OnMouseMove(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = float(xpos);
        lastY = float(ypos);
        firstMouse = false;
    }

    GLfloat xoffset = float(xpos) - lastX;
    GLfloat yoffset = lastY - float(ypos);

    lastX = float(xpos);
    lastY = float(ypos);

    if (g_captureMouse)
        camera.ProcessMouseMove(xoffset, yoffset);
}


void OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(GLfloat(yoffset));
}

void doCameraMovement(Camera& camera, GLfloat deltaTime)
{
    if (keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

GLsizei CreatePlane(GLuint& vao, vec4 ColorofFigure)  // opengl презентация
{
    std::vector<float> vertices = { 20.5f, 0.0f, 20.5f, 1.0f,  
                                 20.5f, 0.0f, -20.5f, 1.0f,
                                -20.5f, 0.0f, 20.5f, 1.0f,
                                -20.5f, 0.0f, -20.5f, 1.0f };

    std::vector<float> normals = { 0.0f, 0.0f, -1.0f, 1.0f,
                                   0.0f, 0.0f, -1.0f, 1.0f,
                                   0.0f, 0.0f, -1.0f, 1.0f,
                                   0.0f, 0.0f, -1.0f, 1.0f };

    std::vector<uint32_t> indices = { 0u, 1u, 2u,
                                      1u, 2u, 3u };  

    std::vector<float> color = { ColorofFigure.x, ColorofFigure.y, ColorofFigure.z, ColorofFigure.w,
                                  ColorofFigure.x, ColorofFigure.y, ColorofFigure.z, ColorofFigure.w,
                                                ColorofFigure.x, ColorofFigure.y, ColorofFigure.z, ColorofFigure.w,
                                                ColorofFigure.x, ColorofFigure.y, ColorofFigure.z, ColorofFigure.w, };
        
    

    GLuint vboVertices, vboIndices, vboNormals;


    GLuint vboColor;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboIndices);

    glBindVertexArray(vao);

    glGenBuffers(1, &vboVertices);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboNormals);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &vboColor);
    glBindBuffer(GL_ARRAY_BUFFER, vboColor);
    glBufferData(GL_ARRAY_BUFFER, color.size() * sizeof(GLfloat), color.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);
   
    glBindVertexArray(0);

    return indices.size();
}

GLsizei CreateSphere(float radius, int numberSlices, GLuint& vao, vec4 ColorofFigure)
{
    int i, j;

    int numberParallels = numberSlices;
    int numberVertices = (numberParallels + 1) * (numberSlices + 1); // вершины задаём
    int numberIndices = numberParallels * numberSlices * 4;

    float angleStep = (2.0f * PI) / ((float)numberSlices);

    std::vector<float> pos(numberVertices * 4, 0.0f);
    std::vector<float> norm(numberVertices * 4, 0.0f);
    std::vector<float> texcoords(numberVertices * 2, 0.0f);
    std::vector<float> color;
    std::vector<int> indices(numberIndices, -1);

    for (i = 0; i < numberParallels + 1; i++)
    {
        for (j = 0; j < numberSlices + 1; j++)
        {
            int vertexIndex = (i * (numberSlices + 1) + j) * 4;
            int normalIndex = (i * (numberSlices + 1) + j) * 4;
            int texCoordsIndex = (i * (numberSlices + 1) + j) * 2;

            pos.at(vertexIndex + 0) = radius * sinf(angleStep * (float)i) * sinf(angleStep * (float)j);
            pos.at(vertexIndex + 1) = radius * cosf(angleStep * (float)i);
            pos.at(vertexIndex + 2) = radius * sinf(angleStep * (float)i) * cosf(angleStep * (float)j);
            pos.at(vertexIndex + 3) = 1.0f;

            norm.at(normalIndex + 0) = pos.at(vertexIndex + 0) / radius;
            norm.at(normalIndex + 1) = pos.at(vertexIndex + 1) / radius;
            norm.at(normalIndex + 2) = pos.at(vertexIndex + 2) / radius;
            norm.at(normalIndex + 3) = 1.0f;

           
           
           

            texcoords.at(texCoordsIndex + 0) = (float)j / (float)numberSlices;
            texcoords.at(texCoordsIndex + 1) = (1.0f - (float)i) / (float)(numberParallels - 1);
        }
    }

 

    int* indexBuf = &indices[0];

    for (i = 0; i < numberParallels; i++)
    {
        for (j = 0; j < numberSlices; j++)
        {
            *indexBuf++ = i * (numberSlices + 1) + j;
            *indexBuf++ = (i + 1) * (numberSlices + 1) + j;
            *indexBuf++ = (i + 1) * (numberSlices + 1) + (j + 1);

            *indexBuf++ = i * (numberSlices + 1) + j;
            *indexBuf++ = (i + 1) * (numberSlices + 1) + (j + 1);
            *indexBuf++ = i * (numberSlices + 1) + (j + 1);

            int diff = int(indexBuf - &indices[0]);
            if (diff >= numberIndices)
                break;
        }
        int     diff = int(indexBuf - &indices[0]);
        if (diff >= numberIndices)
            break;
    }
    
    for (int i = 1; i < pos.size() * 4; i++)
    {
        color.push_back(ColorofFigure.x);
        color.push_back(ColorofFigure.y);
        color.push_back(ColorofFigure.z);
        color.push_back(ColorofFigure.w);

    }

    GLuint vboVertices, vboIndices, vboNormals, vboTexCoords, vboColor;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboIndices);

    glBindVertexArray(vao);

    glGenBuffers(1, &vboVertices);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(GLfloat), &pos[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboNormals);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
    glBufferData(GL_ARRAY_BUFFER, norm.size() * sizeof(GLfloat), &norm[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &vboTexCoords);
    glBindBuffer(GL_ARRAY_BUFFER, vboTexCoords);
    glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(GLfloat), &texcoords[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &vboColor);
    glBindBuffer(GL_ARRAY_BUFFER, vboColor);
    glBufferData(GL_ARRAY_BUFFER, color.size() * sizeof(GLfloat), color.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4* sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(3);


    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);
    
    return indices.size();
}

GLsizei CreateCuboid(float lenght , float width, float height, GLuint& vao, vec4 ColorofFigure) 
{
    std::vector <float> vertex = { 0.0f,0.0f,0.0f,1.0f,  0.0f,lenght,0.0f, 1.0f,  width,lenght,0.0f,1.0f,   width,0.0f,0.0f,1.0f,
                                   width,0.0f,height,1.0f,  width,lenght,height,1.0f,  0.0f,lenght,height,1.0f,  0.0f,0.0f,height,1.0f };
    std::vector <int> indices = { 0,1,2, 0,2,3, 2,3,4, 2,4,5, 4,5,6, 4,6,7, 7,0,4, 0,4,3, 6,7,1, 0,1,7, 1,2,6, 5,2,6 };

    std::vector <float> normal = { 0.0f,0.0f,-1.0f, 0.0f,0.0f,-1.0f, 1.0f,0.0f,0.0f, 1.0f,0.0f,0.0f,
                                  0.0f,0.0f,1.0f, 0.0f,0.0f,1.0f, 0.0f,-1.0f,0.0f, 0.0f,-1.0f,0.0f,
                                  -1.0f,0.0f,0.0f, -1.0f,0.0f,0.0f, 1.0f,0.0f,0.0f, 1.0f,0.0f,0.0f };

    std::vector<float> color = { ColorofFigure.x,ColorofFigure.y,ColorofFigure.z,ColorofFigure.w,
        ColorofFigure.x,ColorofFigure.y,ColorofFigure.z, ColorofFigure.w,
        ColorofFigure.x,ColorofFigure.y,ColorofFigure.z,ColorofFigure.w,
        ColorofFigure.x,ColorofFigure.y,ColorofFigure.z,ColorofFigure.w,
       ColorofFigure.x,ColorofFigure.y,ColorofFigure.z,ColorofFigure.w,
        ColorofFigure.x,ColorofFigure.y,ColorofFigure.z,ColorofFigure.w,
        ColorofFigure.x,ColorofFigure.y,ColorofFigure.z,ColorofFigure.w,
        ColorofFigure.x,ColorofFigure.y,ColorofFigure.z,ColorofFigure.w };

    GLuint vboVertices, vboIndices, vboNormals, vboColor;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboIndices);

    glBindVertexArray(vao);

    glGenBuffers(1, &vboVertices);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(GL_ARRAY_BUFFER, vertex.size() * sizeof(GLfloat), vertex.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboNormals);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
    glBufferData(GL_ARRAY_BUFFER, normal.size() * sizeof(GLfloat), normal.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &vboColor);
    glBindBuffer(GL_ARRAY_BUFFER, vboColor);
    glBufferData(GL_ARRAY_BUFFER, color.size() * sizeof(GLfloat), color.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);
   
    glBindVertexArray(0);

    return indices.size();
}

GLsizei CreateConus(float radius, float height, int numberSlices, GLuint& vao, vec4 ColorOfFigure)
{
    std::vector <float> vertex;//вершины
    std::vector <int> faces;//треугольники по индексам
    std::vector <float> normal;//нормали
    std::vector <float> color;//нормали

    float xpos = 0.0f;
    float ypos = 0.0f;

    float angle = PI * 2.f / float(numberSlices); //делим окружность на секции

    //центр дна
    vertex.push_back(xpos); vertex.push_back(0.0f);
    vertex.push_back(ypos);
    vertex.push_back(1.0f); //w

    //расчёт всех точек дна
    for (int i = 1; i <= numberSlices; i++)
    {
        float newX = radius * sinf(angle * i);
        float newY = -radius * cosf(angle * i);

        //для дна
        vertex.push_back(newX); vertex.push_back(0.0f);
        vertex.push_back(newY);

        vertex.push_back(1.0f); //w
    }

    //координата вершины конуса
    vertex.push_back(xpos); vertex.push_back(height);
    vertex.push_back(ypos);

    vertex.push_back(1.0f); //w

    //ИТОГО: вершины: центр основания, точка основания 1, точка основания 2,
    // и т.д., точка-вершина (четыре координаты)

    //расчёт поверхности дна + нормали
    for (int i = 1; i <= numberSlices; i++)
    {
        faces.push_back(0); //центр основания
        faces.push_back(i); //текущая точка

        if (i != numberSlices) //если не крайняя точка основания
        {
            faces.push_back(i + 1);//то соединяем со следующей по индексу
        }
        else
        {
            faces.push_back(1);//замыкаем с 1ой
        }

        //нормали у дна смотрят вниз
        normal.push_back(0.0f);
        normal.push_back(0.0f);
        normal.push_back(-1.0f);
    }
    //боковые поверхности + нормали
    for (int i = 1; i <= numberSlices; i++)
    {
        int k = 0;//нужно для нормалей

        faces.push_back(i);//текущая

        if (i != numberSlices) //если не крайняя точка основания
        {
            faces.push_back(i + 1);//то соединяем со следующей по индексу
            k = i + 1;
        }
        else
        {
            faces.push_back(1);//замыкаем с 1ой
            k = 1;
        }

        faces.push_back(numberSlices + 1);//вершина

        //расчет нормали к боковой
        float3 a, b, normalVec;
        //вектор а = координаты текущей - координаты вершины
        a.x = vertex[i * 4 - 3] - vertex[vertex.size() - 1 - 3];
        a.y = vertex[i * 4 - 2] - vertex[vertex.size() - 1 - 2];;
        a.z = vertex[i * 4 - 1] - vertex[vertex.size() - 1 - 1];;

        //вектор б = координаты седующей текущей (или 1 при последней итерации)
        // - координаты вершины)
        b.x = vertex[k * 4 - 3] - vertex[vertex.size() - 1 - 3];
        b.x = vertex[k * 4 - 2] - vertex[vertex.size() - 1 - 2];
        b.x = vertex[k * 4 - 1] - vertex[vertex.size() - 1 - 1];

        //нормаль как векторное произведение
        normalVec = cross(a, b);

        //запись нормаль в вектор
        normal.push_back(normalVec.x);
        normal.push_back(normalVec.y);
        normal.push_back(normalVec.z);
    }

    for (int i = 1; i <= numberSlices*4; i++)
    {
        color.push_back(ColorOfFigure.x);
        color.push_back(ColorOfFigure.y);
        color.push_back(ColorOfFigure.z);
        color.push_back(ColorOfFigure.w);

    }
    
   
    GLuint vboVertices, vboIndices, vboNormals, vboColor;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glGenVertexArrays(1, &vao); //
    glGenBuffers(1, &vboIndices);

    glBindVertexArray(vao); //

    glGenBuffers(1, &vboVertices); 
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);  // создаём буффер и помещае массив верши
  
    glBufferData(GL_ARRAY_BUFFER, vertex.size() * sizeof(GLfloat), vertex.data(), GL_STATIC_DRAW); // передаём информацию о в вершинах в OpenGL
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboColor);
    glBindBuffer(GL_ARRAY_BUFFER, vboColor);
    glBufferData(GL_ARRAY_BUFFER, color.size() * sizeof(GLfloat), color.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(3);

    glGenBuffers(1, &vboNormals);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
    glBufferData(GL_ARRAY_BUFFER, normal.size() * sizeof(GLfloat), normal.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
   
   // Атрибут 0. Подробнее об этом будет рассказано в части, посвященной шейдерам.
   // 3,                  // Размер
     //   GL_FLOAT,           // Тип
     //   GL_FALSE,           // Указывает, что значения не нормализованы
     //   0,                  // Шаг
      //  (void*)0            // Смещение массива в буфере
    glEnableVertexAttribArray(1);




    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(int), faces.data(), GL_STATIC_DRAW);
   
   
    glBindVertexArray(0);

    return faces.size();
}

GLsizei CreateCylinder(float radius, float height, int numberSlices, GLuint& vao, vec4 ColorOfFigure)
{

    float angle = 360 / numberSlices; //в градусах


    std::vector<float> position =
    {
       0.0f, 0.0f, 0.0f, 1.0f, //низ
        0.0f, 0.0f + height , 0.0f  , 1.0f, //верх
    };


    std::vector<float> normals =
    { 0.0f, 0.0f, 0.0f, 1.0f,
      1.0f, 1.0f, 0.0f, 1.0f };

    std::vector<float> color;


    std::vector<uint32_t> indices;

    for (int i = 2; i < numberSlices + 2; i++) {
        // нижняя точка
        position.push_back(radius * cos(angle * i * PI / 180));  // x
        position.push_back(0.0f);                              // y
        position.push_back(radius * sin(angle * i * PI / 180)); // z
        position.push_back(1.0f);

        // верхняя точка
        position.push_back(radius * cos(angle * i * PI / 180));  // x
        position.push_back(height);                    // y
        position.push_back(radius * sin(angle * i * PI / 180)); // z
        position.push_back(1.0f);

        normals.push_back(position.at(i + 0) / numberSlices * 8.2);
        normals.push_back(position.at(i + 1) / numberSlices * 8.2);
        normals.push_back(position.at(i + 2) / numberSlices * 8.2);
        normals.push_back(1.0f);
        normals.push_back(position.at(i + 0) / numberSlices * 8.2);
        normals.push_back(position.at(i + 1) / numberSlices * 8.2);
        normals.push_back(position.at(i + 2) / numberSlices * 8.2);
        normals.push_back(1.0f);


    }

    for (int i = 2; i < numberSlices * 2 + 2; i = i + 2) {
        // нижнее основание
        indices.push_back(0);
        indices.push_back(i);
        if ((i) == numberSlices * 2)
        {
            indices.push_back(2);
        }
        else
        {
            indices.push_back(i + 2);
        }
        // верхнее основание
        indices.push_back(1);
        indices.push_back(i + 1);
        if ((i) == numberSlices * 2)
        {
            indices.push_back(3);
        }
        else
        {
            indices.push_back(i + 3);
        }

        indices.push_back(i);
        indices.push_back(i + 1);
        if ((i) == numberSlices * 2)
        {
            indices.push_back(2);
        }
        else
        {
            indices.push_back(i + 2);
        }

        indices.push_back(i + 1);
        if ((i) == numberSlices * 2)
        {
            indices.push_back(2);
            indices.push_back(3);
        }
        else
        {
            indices.push_back(i + 2);
            indices.push_back(i + 3);
        }

    }

    for (int i = 1; i < indices.size()*4; i++)
    {
        color.push_back(ColorOfFigure.x);
        color.push_back(ColorOfFigure.y);
        color.push_back(ColorOfFigure.z);
        color.push_back(ColorOfFigure.w);
    }
   
    GLuint vboVertices, vboIndices, vboNormals, vboColor;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboIndices);

    glBindVertexArray(vao);

    glGenBuffers(1, &vboVertices);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(GL_ARRAY_BUFFER, position.size() * sizeof(GLfloat), position.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboNormals);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &vboColor);
    glBindBuffer(GL_ARRAY_BUFFER, vboColor);
    glBufferData(GL_ARRAY_BUFFER, color.size() * sizeof(GLfloat), color.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    return indices.size();
}

int initGL()
{
    int res = 0;

    //грузим функции opengl через glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    //выводим в консоль некоторую информацию о драйвере и контексте opengl
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    std::cout << "Controls: " << std::endl;
    std::cout << "press right mouse button to capture/release mouse cursor  " << std::endl;
    std::cout << "press spacebar to alternate between shaded wireframe and fill display modes" << std::endl;
    std::cout << "press ESC to exit" << std::endl;

    return 0;
}



int main(int argc, char** argv)
{
    if (!glfwInit())
        return -1;

    //запрашиваем контекст opengl версии 3.3
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL basic sample", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    //регистрируем коллбеки для обработки сообщений от пользователя - клавиатура, мышь..
    glfwSetKeyCallback(window, OnKeyboardPressed);
    glfwSetCursorPosCallback(window, OnMouseMove);
    glfwSetMouseButtonCallback(window, OnMouseButtonClicked);
    glfwSetScrollCallback(window, OnMouseScroll);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (initGL() != 0)
        return -1;

    //Reset any OpenGL errors which could be present for some reason
    GLenum gl_error = glGetError();
    while (gl_error != GL_NO_ERROR)
        gl_error = glGetError();

    //создание шейдерной программы из двух файлов с исходниками шейдеров
    //используется класс-обертка ShaderProgram
    std::unordered_map<GLenum, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = "../shaders/vertex.glsl";
    shaders[GL_FRAGMENT_SHADER] = "../shaders/lambert.frag";
    ShaderProgram lambert(shaders); GL_CHECK_ERRORS;

    ////////////////////////////
    GLuint vaoPlane;
    vec4 ColorofPlane(1.0f, 0.0f, 0.0f, 0.3f);
    GLuint vaoPlane2;
    vec4 ColorofPlane1(0.0f, 0.0f, 1.0f, 0.3f);

    GLuint vaoSphere;
    vec4 CSPH (1.0f, 0.0f, 0.0f, 0.3f);

    GLuint vaoCuboid;
    vec4 ColorOfCuboid(0.0f, 1.0f, 0.0f, 1.0f);

    GLuint vaoConus;
    vec4 ColorOfConus(0.0f, 0.0f, 1.0f, 0.3f);

    GLuint vaoCylinder;
    vec4 ColorOfCylinder(0.0f, 0.5f, 0.5f, 1.0f);



    GLuint vaoForest;
    GLuint vaoBee;
    GLuint vaoConus2;
    ////////////////////////////
    GLsizei sphere = CreateSphere(1.0f, 1000, vaoSphere, CSPH);
    GLsizei Plane = CreatePlane(vaoPlane, ColorofPlane);
    GLsizei Plane2 = CreatePlane(vaoPlane2, ColorofPlane1);
    GLsizei Cuboid = CreateCuboid(0.5f, 0.4f, 0.6f, vaoCuboid, ColorOfCuboid);
    GLsizei Conus = CreateConus(1.0f, 3.0f, 100, vaoConus, ColorOfConus);
    GLsizei Conus2 = CreateConus(1.0f, 3.0f, 100, vaoConus2, ColorOfConus);
    GLsizei Cylinder = CreateCylinder(1.0f, 1.0f, 12, vaoCylinder, ColorOfCylinder);
    GLsizei Forest = CreateConus(1.0f, 3.0f, 72, vaoForest, ColorOfConus);
    GLsizei Bee = CreateSphere(0.5f, 200, vaoBee, CSPH);
    ////////////////////////////////////////////////////////////////////////////////
   map<float,GLsizei> sorted;
   map<float, GLuint> sorted1;
   map<float, float4x4> sorted2;

   struct VAO
   {
       GLsizei gLsizei;
       GLuint glunit;
       float4x4 model;

   };

   map<float, VAO> sort;

    float d;
    float4x4 cuboid1;
    float4x4 sphere1;
    float4x4 Plane1;
    float4x4 Plane12;
    float4x4 Conus1;
    float4x4 Conus21;
    float4x4 Cylinder1;

    glViewport(0, 0, WIDTH, HEIGHT);  GL_CHECK_ERRORS;
    glEnable(GL_DEPTH_TEST);  GL_CHECK_ERRORS;
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
   
    
    float timer = 0.0f;
    //цикл обработки сообщений и отрисовки сцены каждый кадр
    while (!glfwWindowShouldClose(window))
    {
        
       // glDepthMask(GL_TRUE);
        //считаем сколько времени прошло за кадр
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        doCameraMovement(camera, deltaTime);

        //очищаем экран каждый кадр
        glClearColor(0.9f, 0.95f, 0.97f, 1.0f); GL_CHECK_ERRORS;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); GL_CHECK_ERRORS;

        lambert.StartUseShader(); GL_CHECK_ERRORS;
        float4x4 view = camera.GetViewMatrix();
        float4x4 projection = projectionMatrixTransposed(camera.zoom, float(WIDTH) / float(HEIGHT), 0.1f, 1000.0f);
        float4x4 model;
        VAO buffer;

        lambert.SetUniform("view", view);       GL_CHECK_ERRORS;
        lambert.SetUniform("projection", projection); GL_CHECK_ERRORS;

    

      

        glBindVertexArray(vaoCylinder); GL_CHECK_ERRORS;
        {
            Cylinder1 = transpose(mul(translate4x4(float3(3.2f, 8.0f, 14.0f)), scale4x4(float3(0.6f, 1.5f, 0.6f))));
           // lambert.SetUniform("model", Cylinder1); GL_CHECK_ERRORS;
            d = sqrt((camera.pos.x - 3.2f) * (camera.pos.x - 3.2f) + (camera.pos.y - 8.0f) * (camera.pos.y - 8.0f) + (camera.pos.z - 14.0f) * (camera.pos.z - 14.0f));
            buffer.gLsizei = Cylinder;
            buffer.glunit = vaoCylinder;
            buffer.model = Cylinder1;
            sort[d] = buffer;
           // glDrawElements(GL_TRIANGLE_STRIP, Cylinder, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;
        }
        
     
        glBindVertexArray(vaoSphere);
        {   
            
            sphere1 = transpose(mul(translate4x4(float3(-1.0f, 9.0f, 10.0f)), scale4x4(float3(1.0f, 1.0f, 1.0f)))); //  начальная тока сплющивание 
             d = sqrt((camera.pos.x +1.0f) * (camera.pos.x+ 1.0f) + (camera.pos.y - 9.0f) * (camera.pos.y - 9.0f) + (camera.pos.z - 10.0f) * (camera.pos.z - 10.0f));
           //  lambert.SetUniform("model", sphere1); GL_CHECK_ERRORS;

           
          buffer.gLsizei = sphere; 
          buffer.glunit = vaoSphere;
          buffer.model = sphere1;
          sort[d] = buffer;
       //   glDrawElements(GL_TRIANGLE_STRIP, sphere, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;
          }
       
       


       
        ///////////////////     Плоскость
        glBindVertexArray(vaoPlane);
        {   
            Plane1 = transpose(translate4x4(float3(0.0f, -6.0f, 0.0f)));
          d = sqrt((camera.pos.x - 0.0f) * (camera.pos.x - 0.0f) + (camera.pos.y + 6.0f) * (camera.pos.y + 6.0f) + (camera.pos.z - 0.0f) * (camera.pos.z - 0.0f));

           buffer.gLsizei = Plane;
           buffer.glunit = vaoPlane;
           buffer.model = Plane1;
           sort[d] = buffer;
        }


        glBindVertexArray(vaoPlane2);
        {
            Plane12 = transpose(translate4x4(float3(0.0f, -8.0f, 0.0f)));
            d = sqrt((camera.pos.x - 0.0f) * (camera.pos.x - 0.0f) + (camera.pos.y+ 8.0f) * (camera.pos.y + 8.0f) + (camera.pos.z - 0.0f) * (camera.pos.z - 0.0f));

            buffer.gLsizei = Plane2;
            buffer.glunit = vaoPlane2;
            buffer.model = Plane12;
            sort[d] = buffer;
        }
        
     
        glBindVertexArray(vaoForest);
        {
            for (int i = 1; i < 700; i++)
            {
                float4x4 model1 = transpose(mul(translate4x4(float3(sin(20 * i) * 19.0f, -5.99f, cos(i) * 19.0f)), scale4x4(float3(0.5f, (0.7 + sin(i / 2) / 3 * pow((-1), i)) * 1.0f, 0.5f))));
                lambert.SetUniform("model", model1); GL_CHECK_ERRORS;
                glDrawElements(GL_TRIANGLE_STRIP, Forest, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;
            }
        }

        ///////////////////     Галактика
        glBindVertexArray(vaoBee);
        {

            float4x4 modelBee;
            for (int i = 1; i < 10; i++)
            {
               

                modelBee = transpose(mul(translate4x4(float3((-1.0f + i + sin(timer * 50)), -2.0f + i * 5 + cos(timer * 5), 1.0f)), scale4x4(float3(2.0f, 2.0f, 2.0f))));
                lambert.SetUniform("model", modelBee); GL_CHECK_ERRORS;
                glDrawElements(GL_TRIANGLE_STRIP, Bee, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;


                modelBee = transpose(mul(translate4x4(float3((-1.0f + i + sin(timer * 50)), -2.0f + i * 5 + cos(timer * 5), 1.0f)), scale4x4(float3(5.0f, 0.1f, 5.0f))));
                lambert.SetUniform("model", modelBee); GL_CHECK_ERRORS;
                glDrawElements(GL_TRIANGLE_STRIP, Bee, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;

            }
        }


     


    
      
        glBindVertexArray(vaoConus);
        {
            Conus1 = transpose(mul(translate4x4(float3(1.5f, 8.0f, 16.0f)), scale4x4(float3(1.0f, 0.5f, 1.0f))));
           // lambert.SetUniform("model", Conus1); GL_CHECK_ERRORS;
            d = sqrt((camera.pos.x - 1.5f) * (camera.pos.x - 1.5f) + (camera.pos.y - 8.0f) * (camera.pos.y - 8.0f) + (camera.pos.z - 16.0f) * (camera.pos.z - 16.0f));


           buffer.gLsizei = Conus;


            buffer.glunit = vaoConus;
            buffer.model = Conus1;
            sort[d] = buffer;
          //  glDrawElements(GL_TRIANGLE_STRIP, Conus, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;
           
        }

   
     

    /*    glBindVertexArray(vaoConus2);
        {
            Conus21 = transpose(mul(translate4x4(float3(2.0f, 8.0f, 16.01f)), scale4x4(float3(1.0f, 0.5f, 1.0f))));
           
            d = sqrt((camera.pos.x - 2.0f) * (camera.pos.x - 2.0f) + (camera.pos.y - 8.0f) * (camera.pos.y - 8.0f) + (camera.pos.z - 16.0f) * (camera.pos.z - 16.0f));


            buffer.gLsizei = Conus2;
           buffer.glunit = vaoConus2;
            buffer.model = Conus21;
            sort[d] = buffer;
            
        }*/


    for (map<float, VAO>::reverse_iterator it = sort.rbegin(); it != sort.rend(); ++it)
    {
        glBindVertexArray(it->second.glunit);
        {
            lambert.SetUniform("model", it->second.model); GL_CHECK_ERRORS;

            glDrawElements(GL_TRIANGLE_STRIP, it->second.gLsizei, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS; }

    }
    sort.clear();


       

     
        timer += 0.001f;
  
        glBindVertexArray(0); GL_CHECK_ERRORS;

        lambert.StopUseShader(); GL_CHECK_ERRORS;
        glfwSwapBuffers(window);
    }


    //glDeleteVertexArrays(1, &vaoSphere);

    glfwTerminate();
    return 0;
}


