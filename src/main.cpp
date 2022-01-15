#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "rg/Error.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

void renderQuad();
void renderCube();

unsigned int loadTexture(char const * path, bool gammaCorrection);

unsigned int loadTexture(const char *path);

unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 700;
//bool bloom = true;
bool bloomKeyPressed = false;
bool spotKeyPressed=false;
//float exposure =0.5;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;


// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};
//std::string glslIdentifierPrefix;


struct PointLight {
    glm::vec3 position;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};



struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    glm::vec3 islandPosition;
    bool CameraMouseMovementUpdateEnabled = true;
    bool spotlight=true;
    bool bloom = true;

    float exposure =0.5;

    //Light pointLights[2];
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << spotlight << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n'
        << bloom << '\n'
        << exposure << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> spotlight
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z
           >> bloom
           >> exposure;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CW);


    // build and compile shaders
    // ----------------------

    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader shader("resources/shaders/bloom.vs", "resources/shaders/bloom.fs");
    Shader shaderLight("resources/shaders/bloom.vs", "resources/shaders/lb.fs");
     Shader shaderBlur("resources/shaders/blur.vs", "resources/shaders/blur.fs");
     Shader shaderBloomFinal("resources/shaders/bloom_final.vs", "resources/shaders/bloom_final.fs");
    Shader shaderBlending("resources/shaders/blending.vs", "resources/shaders/blending.fs");



         Model islan("resources/objects/islan/Small_Tropical_Island.obj");
          islan.SetShaderTextureNamePrefix("material.");

         Model heli("resources/objects/heli/ah64d.obj");
          heli.SetShaderTextureNamePrefix("material.");


          unsigned int hdrFBO;
          glGenFramebuffers(1, &hdrFBO);
          glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
          // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
          unsigned int colorBuffers[2];
          glGenTextures(2, colorBuffers);
          for (unsigned int i = 0; i < 2; i++)
          {
              glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
              glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
              // attach texture to framebuffer
              glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
          }
          // create and attach depth buffer (renderbuffer)
          unsigned int rboDepth;
          glGenRenderbuffers(1, &rboDepth);
          glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
          glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
          glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
          // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
          unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
          glDrawBuffers(2, attachments);
          // finally check if framebuffer is complete
          if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
              std::cout << "Framebuffer not complete!" << std::endl;
          glBindFramebuffer(GL_FRAMEBUFFER, 0);

          // ping-pong-framebuffer for blurring
          unsigned int pingpongFBO[2];
          unsigned int pingpongColorbuffers[2];
          glGenFramebuffers(2, pingpongFBO);
          glGenTextures(2, pingpongColorbuffers);
          for (unsigned int i = 0; i < 2; i++)
          {
              glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
              glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
              glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
              glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
              // also check if framebuffers are complete (no need for depth buffer)
              if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                  std::cout << "Framebuffer not complete!" << std::endl;
          }
    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
    unsigned int transparentTexture=0;
    GLCALL(transparentTexture = loadTexture(FileSystem::getPath("resources/textures/grass.png").c_str()));

    vector<glm::vec3> vegetation
            {
                   // glm::vec3(6.77f, 1.0f, 4.11f),
                    glm::vec3(8.44f, 1.0f, 5.53f),
                    glm::vec3(11.72f, 1.82f, 4.85f),
                    glm::vec3(7.17f,0.60f,1.90f),
                    glm::vec3(0)

            };
          float skyboxVertices[] = {
                  // positions
                  -1.0f,  1.0f, -1.0f,
                  -1.0f, -1.0f, -1.0f,
                  1.0f, -1.0f, -1.0f,
                  1.0f, -1.0f, -1.0f,
                  1.0f,  1.0f, -1.0f,
                  -1.0f,  1.0f, -1.0f,

                  -1.0f, -1.0f,  1.0f,
                  -1.0f, -1.0f, -1.0f,
                  -1.0f,  1.0f, -1.0f,
                  -1.0f,  1.0f, -1.0f,
                  -1.0f,  1.0f,  1.0f,
                  -1.0f, -1.0f,  1.0f,

                  1.0f, -1.0f, -1.0f,
                  1.0f, -1.0f,  1.0f,
                  1.0f,  1.0f,  1.0f,
                  1.0f,  1.0f,  1.0f,
                  1.0f,  1.0f, -1.0f,
                  1.0f, -1.0f, -1.0f,

                  -1.0f, -1.0f,  1.0f,
                  -1.0f,  1.0f,  1.0f,
                  1.0f,  1.0f,  1.0f,
                  1.0f,  1.0f,  1.0f,
                  1.0f, -1.0f,  1.0f,
                  -1.0f, -1.0f,  1.0f,

                  -1.0f,  1.0f, -1.0f,
                  1.0f,  1.0f, -1.0f,
                  1.0f,  1.0f,  1.0f,
                  1.0f,  1.0f,  1.0f,
                  -1.0f,  1.0f,  1.0f,
                  -1.0f,  1.0f, -1.0f,

                  -1.0f, -1.0f, -1.0f,
                  -1.0f, -1.0f,  1.0f,
                  1.0f, -1.0f, -1.0f,
                  1.0f, -1.0f, -1.0f,
                  -1.0f, -1.0f,  1.0f,
                  1.0f, -1.0f,  1.0f
          };

           stbi_set_flip_vertically_on_load(true);
          unsigned int skyboxVAO, skyboxVBO;
          glGenVertexArrays(1, &skyboxVAO);
          glGenBuffers(1, &skyboxVBO);
          glBindVertexArray(skyboxVAO);
          glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
          glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
          glEnableVertexAttribArray(0);
          glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

          // load textures
          vector<std::string> faces
                  {
                          FileSystem::getPath("resources/textures/skybox/skyboxbak/right.png"),
                          FileSystem::getPath("resources/textures/skybox/skyboxbak/left.png"),
                          FileSystem::getPath("resources/textures/skybox/skyboxbak/top.png"),
                          FileSystem::getPath("resources/textures/skybox/skyboxbak/bottom.png"),
                          FileSystem::getPath("resources/textures/skybox/skyboxbak/front.png"),
                          FileSystem::getPath("resources/textures/skybox/skyboxbak/back.png")
                  };
          unsigned int cubemapTexture = loadCubemap(faces);
    stbi_set_flip_vertically_on_load(false);


          // render loop
          // -----------

          std::vector<glm::vec3> lightPositions;
          lightPositions.push_back(glm::vec3(1,1,1));
          lightPositions.push_back(glm::vec3(2,2,2));

          std::vector<glm::vec3> lightAmbients;
          lightAmbients.push_back(glm::vec3(0.05,0.05,0.05));
          lightAmbients.push_back(glm::vec3(0.05,0.05,0.05));

          std::vector<glm::vec3> lightDiffuse;
          lightDiffuse.push_back(glm::vec3(0.8,0.8,0.8));
          lightDiffuse.push_back(glm::vec3(0.8,0.8,0.8));

          std::vector<glm::vec3> lightSpecular;
          lightSpecular.push_back(glm::vec3(1,1,1));
          lightSpecular.push_back(glm::vec3(1,1,1));

          std::vector<float> lightConstant;
          lightConstant.push_back(1.0f);
          lightConstant.push_back(1.0f);

          std::vector<float> lightLinear;
          lightLinear.push_back(0.09f);
          lightLinear.push_back(0.09f);

          std::vector<float> lightQuadratic;
          lightQuadratic.push_back(0.032f);
          lightQuadratic.push_back(0.032f);


    shader.use();

              shader.setVec3("pointLight1.ambient", lightAmbients[0]);
              shader.setVec3("pointLight1.diffuse", lightDiffuse[0]);
              shader.setVec3("pointLight1.specular", lightSpecular[0]);
              shader.setFloat("pointLight1.constant", lightConstant[0]);
              shader.setFloat("pointLight1.linear", lightLinear[0]);
              shader.setFloat("pointLight1.quadratic", lightQuadratic[0]);


    shader.setVec3("pointLight2.ambient", lightAmbients[1]);
    shader.setVec3("pointLight2.diffuse", lightDiffuse[1]);
    shader.setVec3("pointLight2.specular", lightSpecular[1]);
    shader.setFloat("pointLight2.constant", lightConstant[1]);
    shader.setFloat("pointLight2.linear", lightLinear[1]);
    shader.setFloat("pointLight2.quadratic", lightQuadratic[1]);

          shader.setVec3("dirLight.direction", -0.35f, 0.0f, -1.0f);
          shader.setVec3("dirLight.ambient", 0.005f, 0.005f, 0.020f);
          shader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.6f);
          shader.setVec3("dirLight.specular", 0.2f, 0.2f, 0.1f);
          shader.setBool("spot",programState->spotlight);
          shader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
          shader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
          shader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
          shader.setFloat("spotLight.constant", 1.0f);
          shader.setFloat("spotLight.linear", 0.09);
          shader.setFloat("spotLight.quadratic", 0.032);
          shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
          shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));



          //  shader.use();

          shaderBlur.use();
          shaderBlur.setInt("image", 0);
          shaderBloomFinal.use();
          shaderBloomFinal.setInt("scene", 0);
          shaderBloomFinal.setInt("bloomBlur", 1);
          shaderBlending.use();
          shaderBlending.setInt("texture1",0);

          while (!glfwWindowShouldClose(window)) {
              // per-frame time logic
              // --------------------

              float currentFrame = glfwGetTime();
              deltaTime = currentFrame - lastFrame;
              lastFrame = currentFrame;

              // input
              // -----
              processInput(window);


              // render
              // ------
              //glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
              glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
              glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

              // 1. render scene into floating point framebuffer
              // -----------------------------------------------
              glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
              glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
              glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
              glm::mat4 view = programState->camera.GetViewMatrix();
              glm::mat4 model = glm::mat4(1.0f);


              glm::vec3 pos0 = glm::vec3(4.0 * cos(currentFrame), 10.0f, 4.0 * sin(currentFrame));
              glm::vec3 pos1 = glm::vec3(5.0 * sin(currentFrame)+2, 5.0f+3, 5.0 * cos(currentFrame)+1);

              lightPositions[0]=pos0+glm::vec3(0,1,0);
              lightPositions[1]=pos1+glm::vec3(0,1,0);

              shader.use();
              shader.setMat4("projection", projection);
              shader.setMat4("view", view);
              shader.setVec3("pointLight1.position", lightPositions[0]);
              shader.setVec3("pointLight2.position", lightPositions[1]);
              shader.setVec3("viewPos", programState->camera.Position);

              shader.setBool("spot",programState->spotlight);
              shader.setVec3("spotLight.position", programState->camera.Position);
              shader.setVec3("spotLight.direction", programState->camera.Front);

              shaderLight.use();
              shaderLight.setMat4("projection", projection);
              shaderLight.setMat4("view", view);



              shader.use();

              //island
              model = glm::translate(model, programState->islandPosition+glm::vec3(0,6,0));
              model = glm::scale(model, glm::vec3(0.1f));
              shader.setMat4("model", model);
              islan.Draw(shader);


              //heli
              model = glm::mat4(1.0f);
              model=glm::translate(model,pos0+glm::vec3(0,6,0));
              model = glm::scale(model, glm::vec3(0.4f));
              model = glm::rotate(model,-currentFrame, glm::vec3(0,1,0));
              shader.setMat4("model", model);
              heli.Draw(shader);


              model = glm::mat4(1.0f);
              model=glm::translate(model,pos1+glm::vec3(0,6,0));
              model = glm::scale(model, glm::vec3(0.4f));
              model=glm::rotate(model,glm::radians(90.0f),glm::vec3(0,1,0));
              model = glm::rotate(model, currentFrame, glm::vec3(0,1,0));
              shader.setMat4("model", model);
              heli.Draw(shader);

              // vegetation
              glDisable(GL_CULL_FACE);
              shaderBlending.use();
              shaderBlending.setMat4("projection", projection);
              shaderBlending.setMat4("view", view);
              glBindVertexArray(transparentVAO);
              glBindTexture(GL_TEXTURE_2D, transparentTexture);
              for (unsigned int i = 0; i < vegetation.size(); i++)
              {
                  model = glm::mat4(1.0f);
                  model = glm::translate(model, vegetation[i]+glm::vec3(0,6,0));
                  shaderBlending.use();
                  shaderBlending.setMat4("model", model);
                  glDrawArrays(GL_TRIANGLES, 0, 6);
              }
              glEnable(GL_CULL_FACE);
              glCullFace(GL_FRONT);
              glFrontFace(GL_CW);

              //skybox
              glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
              skyboxShader.use();
              glm::mat4 view2 = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
              skyboxShader.setMat4("view", view2);
              skyboxShader.setMat4("projection", projection);
              // skybox cube
              glBindVertexArray(skyboxVAO);
              glActiveTexture(GL_TEXTURE0);
              glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
              glDrawArrays(GL_TRIANGLES, 0, 36);
              glBindVertexArray(0);
              glDepthFunc(GL_LESS);


              //lights
              shaderLight.use();
              for (unsigned int i = 0; i < lightPositions.size(); i++)
              {
                  model = glm::mat4(1.0f);
                  model = glm::translate(model, glm::vec3(lightPositions[i])+glm::vec3(0,6,0));
                  model = glm::scale(model, glm::vec3(0.14f));
                  shaderLight.setMat4("model", model);
                  shaderLight.setVec3("lightColor", glm::vec3(30,30,30));
                  renderCube();
             }

                //blur
              glBindFramebuffer(GL_FRAMEBUFFER, 0);
              bool horizontal = true, first_iteration = true;
                 unsigned int amount = 10;
                shaderBlur.use();
                for (unsigned int i = 0; i < amount; i++)
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
                    shaderBlur.setInt("horizontal", horizontal);
                    glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
                    renderQuad();
                    horizontal = !horizontal;
                    if (first_iteration)
                        first_iteration=false;
                }
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                // 3. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
                // --------------------------------------------------------------------------------------------------------------------------
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                shaderBloomFinal.use();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
                shaderBloomFinal.setInt("bloom", programState->bloom);
                shaderBloomFinal.setFloat("exposure", programState->exposure);
                renderQuad();
              std::cout << "bloom: " << (programState->bloom ? "on" : "off") << "| exposure: " << programState->exposure << std::endl;

              if (programState->ImGuiEnabled)
                  DrawImGui(programState);

              // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
              // -------------------------------------------------------------------------------
              glfwSwapBuffers(window);
              glfwPollEvents();
          }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        programState->camera.MovementSpeed*=1.1;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        programState->camera.MovementSpeed/=1.1;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !bloomKeyPressed)
    {
        programState->bloom = !programState->bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (programState->exposure > 0.0f)
            programState->exposure -= 0.001f;
        else
            programState->exposure = 0.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        programState->exposure += 0.001f;
    }

    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS&&!spotKeyPressed)
    {
        programState->spotlight=!programState->spotlight;
        spotKeyPressed=true;
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE)
    {
        spotKeyPressed=false;
    }

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = (xpos - lastX);
    float yoffset = (lastY - ypos); // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            programState->CameraMouseMovementUpdateEnabled = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
        if (key == GLFW_KEY_P && action == GLFW_PRESS) {
            programState->spotlight=!programState->spotlight;
    }


}
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
unsigned int loadTexture(char const * path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            //internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            internalFormat=GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}


unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube() {
    // initialize (if necessary)

    if (cubeVAO == 0) {
        float vertices[] = {
                // back face

               -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
                0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
                0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
                0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
                -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
                -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,


                -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
                0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
                0.5f,  -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
                0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
                -0.5f,  -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
                -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,


                -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
               -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
                -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
                -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
               -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
                -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,


                0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
                0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
                0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
                0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
                0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
                0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,


               -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
                0.5f, -0.5f, 0.5f,  0.0f, -1.0f,  0.0f,
                0.5f, -0.5f,  -0.5f,  0.0f, -1.0f,  0.0f,
                0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
                -0.5f, -0.5f,  -0.5f,  0.0f, -1.0f,  0.0f,
                -0.5f, -0.5f, 0.5f,  0.0f, -1.0f,  0.0f,


                -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
                0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
                0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
                0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
                -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
                -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f


        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) (3 * sizeof(float)));


        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}


unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        std::cout << "Texture loaded: " << path << std::endl;
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
