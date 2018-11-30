//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   Sons of War
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

// Header de tempo
#include<time.h>




// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};

// Estrutura que representa uma elevação no mapa
struct Plateau
{
    glm::vec3 position;     // Posição do centro do planalto
    glm::vec3 scale;        // Escala das medidas do planalto
    glm::vec2 bottom_limit; // Menores coordenadas de área pertencentes ao planalto
    glm::vec2 top_limit;    // Maiores coordenadas de área pertencentes ao planalto
};

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging
void BuildMeshes(int argc, char* argv[]);

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Funções de personagens
void CreateCharacters(glm::vec3 land_size);
void DrawCharacters();

// Funções de textura.
void LoadTextureImage(const char* filename);
GLint bbox_min_uniform;
GLint bbox_max_uniform;

void PassTurn();

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    void*        first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    int          num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint object_id_uniform;

// Classes
class Lookat_Camera{
public:
    //Vetores
    glm::vec4 position;     // Ponto "c", centro da câmera
    glm::vec4 lookat;   // Ponto "l", para onde a câmera (look-at) estará sempre olhando
    glm::vec4 view;         // Vetor "view", sentido para onde a câmera está virada
    glm::vec4 up;           // Vetor "up" fixado para apontar para o "céu" (eito Y global)
    //Ângulos
    float theta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
    float phi = 0.0f;   // Ângulo em relação ao eixo Y
    // Outras vars
    float distance = 3.5f; // Distância da câmera para a origem
    //Parâmetros
    float speed;
    float nearplane;  // Posição do "near plane"
    float farplane; // Posição do "far plane"

    //Funções
    void init();
    void move(glm::vec4 direction);
    void look(float dtheta, float dphi);
    void update_camera();
    void set_nearplane(float near_value);
    void set_farplane(float far_value);
};

class Free_Camera{
public:
    // Vetores
    glm::vec4 position;
    glm::vec4 view;
    glm::vec4 up;
    glm::vec4 right;
    glm::vec4 v;
    // Ângulos
    float phi;
    // Parâmetros
    float speed;
    // Near e Far
    float nearplane = -0.1f;
    float farplane  = -10.0f;

    void init(glm::vec4 pos, glm::vec4 origin);
    void init_char(glm::vec4 pos, glm::vec4 origin);
    void move(glm::vec4 direction);
    void move2D(glm::vec4 direction);
    void rotate(glm::vec4 axis, float angle);
    void look(float dtheta, float dphi);
};

class Scenary{
public:
    glm::vec3 land_size;            // Escala das medidas da terra
    std::vector<Plateau> plateaus;  // Lista de planaltos

    void build();
    void draw();
    float heigth(glm::vec4 dot);
};

#define SPEARMAN    1
#define GUARDIAN    2
#define ARCHER      3
class Character{
public:
    float max_hp;
    float current_hp;
    float max_movement;
    float remaining_movement;
    int max_actions;
    int remaining_actions;
    float range;
    float damage;
    int team;
    int initiative;
    glm::vec4 position;
    int role;
    Free_Camera camera;
    glm::vec4 facing_vector;

    void attack();
    void init_attributes(int type);
    void take_damage(float delta){
        current_hp = current_hp - delta;
        if (current_hp > max_hp)
            current_hp = max_hp;
        else if (current_hp < 0)
            current_hp = 0;
    }
    void end_turn() {
        remaining_movement = max_movement;
        remaining_actions = max_actions;
    }
    void set_team(int team_number) {
        team = team_number;
    }
    void draw();
    void move();
    void moveFP(glm::vec4 direction);

};

bool moveFoward = false;
bool moveBackwards = false;
bool moveLeft = false;
bool moveRight = false;
bool rotateLeft = false;
bool rotateRight = false;

bool panLookatRight = false;
bool panLookatLeft = false;
bool panLookatUp = false;
bool panLookatDown = false;

// Número de texturas carregadas pela função LoadTextureImage() - LAB4
GLuint g_NumLoadedTextures = 0;

// Câmeras
Lookat_Camera lookat_camera;
Free_Camera free_camera;

#define FIRST_PERSON    0
#define THIRD_PERSON    1
#define FREE_CAM        2
int cam_mode;

// Cenário
Scenary scenary;

// Characters
std::vector<Character> characters;
int active_character;

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Inicializamos a semente random
    srand(time(0));

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    glfwWindowHint(GLFW_SAMPLES, 4);    // Criando buffer de multisample para anti-aliasing
    window = glfwCreateWindow(800, 600, "INF01047 - Sons of War", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 217-219 do documento "Aula_03_Rendering_Pipeline_Grafico.pdf".
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/grass_dirt_texture.bmp");      // TextureImage0
    LoadTextureImage("../../data/water_texture.bmp"); // TextureImage1

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    BuildMeshes(argc, argv);

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slide 108 do documento "Aula_09_Projecoes.pdf".
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 22-34 do documento "Aula_13_Clipping_and_Culling.pdf".
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Variáveis auxiliares utilizadas para chamada à função
    // TextRendering_ShowModelViewProjection(), armazenando matrizes 4x4.
    glm::mat4 the_projection;
    glm::mat4 the_model;
    glm::mat4 the_view;

    // Iniciando câmera lookat
    lookat_camera.init();
    // Iniciando free câmera
    glm::vec4 origin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    free_camera.init(lookat_camera.position, origin);

    // Construindo o cenário
    scenary.build();

    // Criamos os personagens
    CreateCharacters(scenary.land_size);

    // Iniciando com 1º personagem
    active_character = 0;

    // Câmera começa em 3º pessoa
    cam_mode = THIRD_PERSON;

    // Ficamos em loop, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(program_id);

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.
        glm::mat4 view;
        switch(cam_mode){
        case THIRD_PERSON:
            view = Matrix_Camera_View(lookat_camera.position, lookat_camera.view, lookat_camera.up);
            break;
        case FIRST_PERSON:
            view = Matrix_Camera_View(characters[active_character].camera.position, characters[active_character].camera.view, characters[active_character].camera.up);
            break;
        case FREE_CAM:
            view = Matrix_Camera_View(free_camera.position, free_camera.view, free_camera.up);
            break;
        }

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slide 227 do documento "Aula_09_Projecoes.pdf".
            float field_of_view = 3.141592 / 2.4f;
            switch(cam_mode){
            case THIRD_PERSON:
                projection = Matrix_Perspective(field_of_view, g_ScreenRatio, lookat_camera.nearplane, lookat_camera.farplane);
                break;
            case FIRST_PERSON:
                projection = Matrix_Perspective(field_of_view, g_ScreenRatio, characters[active_character].camera.nearplane, characters[active_character].camera.farplane);
                break;
            case FREE_CAM:
                projection = Matrix_Perspective(field_of_view, g_ScreenRatio, free_camera.nearplane, free_camera.farplane);
                break;
            }
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slide 236 do documento "Aula_09_Projecoes.pdf".
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            switch(cam_mode){
            case THIRD_PERSON:
                projection = Matrix_Orthographic(l, r, b, t, lookat_camera.nearplane, lookat_camera.farplane);
                break;
            case FIRST_PERSON:
                projection = Matrix_Orthographic(l, r, b, t, characters[active_character].camera.nearplane, characters[active_character].camera.farplane);
                break;
            case FREE_CAM:
                projection = Matrix_Orthographic(l, r, b, t, free_camera.nearplane, free_camera.farplane);
                break;
            }
        }

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        #define LAND        0
        #define WATER       1
        #define CHAR_TEAM_1 2
        #define CHAR_TEAM_2 3

        // Desenhamos o cenário
        scenary.draw();

        // Desenhamos os personagens
        DrawCharacters();

        // Controle de Movimentos
        glm::vec4 direction = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec4 foward_vec = glm::vec4(lookat_camera.view.x, 0.0f, lookat_camera.view.z, 0.0f);
        glm::vec4 right_vec = Matrix_Rotate_Y(-3.1415/2)*foward_vec;
        switch(cam_mode){
        case THIRD_PERSON:
            if ( panLookatRight )
                lookat_camera.move(right_vec);

            if ( panLookatLeft )
                lookat_camera.move(-right_vec);

            if ( panLookatUp )
                lookat_camera.move(foward_vec);

            if ( panLookatDown )
                lookat_camera.move(-foward_vec);

            if ( rotateLeft ){
                characters[active_character].facing_vector = characters[active_character].facing_vector*Matrix_Rotate_Y(-0.1);
                characters[active_character].camera.view = normalize(characters[active_character].facing_vector);
            }

            if ( rotateRight ){
                characters[active_character].facing_vector = characters[active_character].facing_vector*Matrix_Rotate_Y(0.1);
                characters[active_character].camera.view = normalize(characters[active_character].facing_vector);
            }

            if ( moveFoward )
                characters[active_character].move();

            break;
        case FIRST_PERSON:
            if (moveFoward){
                characters[active_character].moveFP(characters[active_character].camera.view);
            }

            if (moveBackwards){
                characters[active_character].moveFP(-characters[active_character].camera.view);
            }

            if (moveLeft){
                characters[active_character].moveFP(-characters[active_character].camera.right);
            }

            if (moveRight){
                characters[active_character].moveFP(characters[active_character].camera.right);
            }
            break;
        case FREE_CAM:
            if (moveFoward)
                free_camera.move(free_camera.view);

            if (moveBackwards)
                free_camera.move(-free_camera.view);

            if (moveLeft)
                free_camera.move(-free_camera.right);

            if (moveRight)
                free_camera.move(free_camera.right);
            break;
        }

        // Testa o final do turno
        if (characters[active_character].remaining_movement <= 0 && characters[active_character].remaining_actions <= 0)
            PassTurn();

        // Pegamos um vértice com coordenadas de modelo (0.5, 0.5, 0.5, 1) e o
        // passamos por todos os sistemas de coordenadas armazenados nas
        // matrizes the_model, the_view, e the_projection; e escrevemos na tela
        // as matrizes e pontos resultantes dessas transformações.
        //glm::vec4 p_model(0.5f, 0.5f, 0.5f, 1.0f);
        //TextRendering_ShowModelViewProjection(window, projection, view, model, p_model);

        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        TextRendering_ShowEulerAngles(window);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);


    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)g_VirtualScene[object_name].first_index
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 217-219 do documento "Aula_03_Rendering_Pipeline_Grafico.pdf".
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( program_id != 0 )
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform           = glGetUniformLocation(program_id, "model"); // Variável da matriz "model"
    view_uniform            = glGetUniformLocation(program_id, "view"); // Variável da matriz "view" em shader_vertex.glsl
    projection_uniform      = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    object_id_uniform       = glGetUniformLocation(program_id, "object_id"); // Variável "object_id" em shader_fragment.glsl
    bbox_min_uniform        = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform        = glGetUniformLocation(program_id, "bbox_max");


    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
   // glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    glUseProgram(0);


}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gourad, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            // PREENCHA AQUI o cálculo da normal de um triângulo cujos vértices
            // estão nos pontos "a", "b", e "c", definidos no sentido anti-horário.
            glm::vec4 ab = a - b;
            glm::vec4 cb = c - b;
            const glm::vec4  n = crossproduct(cb, ab);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = (void*)first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula (slides 33-44 do documento "Aula_07_Transformacoes_Geometricas_3D.pdf").
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slide 227 do documento "Aula_09_Projecoes.pdf".
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    // Pegando tamanho da tela
    int height, width;
    glfwGetWindowSize(window, &width, &height);

    switch(cam_mode){
    case THIRD_PERSON:
        if ( xpos > 0.98*width )
            panLookatRight = true;
        else
            panLookatRight = false;

        if ( xpos < 0.02*width )
            panLookatLeft = true;
        else
            panLookatLeft = false;

        if ( ypos > 0.98*height )
            panLookatDown = true;
        else
            panLookatDown = false;

        if ( ypos < 0.02*height )
            panLookatUp = true;
        else
            panLookatUp = false;
    }

    if (g_LeftMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
        glm::vec4 view2D;

        switch(cam_mode){
        case THIRD_PERSON:
            lookat_camera.look(dx,dy);
            break;
        case FIRST_PERSON:
            characters[active_character].camera.look(dx, dy);
            view2D = glm::vec4(characters[active_character].camera.view.x, 0.0f, characters[active_character].camera.view.z, 0.0f);
            view2D = normalize(view2D);
            characters[active_character].facing_vector = view2D;
            break;
        case FREE_CAM:
            free_camera.look(dx, dy);
            break;
        }

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_RightMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_ForearmAngleZ -= 0.01f*dx;
        g_ForearmAngleX += 0.01f*dy;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_TorsoPositionX += 0.01f*dx;
        g_TorsoPositionY -= 0.01f*dy;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    lookat_camera.distance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = 0.1;
    if (lookat_camera.distance < 0.4)
        lookat_camera.distance = 0.4;

    lookat_camera.update_camera();
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // ==============
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ==============

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuário apertar a tecla espaço, pulamos o turno.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        PassTurn();
    /*{
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
        g_ForearmAngleX = 0.0f;
        g_ForearmAngleZ = 0.0f;
        g_TorsoPositionX = 0.0f;
        g_TorsoPositionY = 0.0f;
    }*/

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }

    // Tecla F = alterna entre primeira e terceira pessoa
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        if (cam_mode == THIRD_PERSON)
            cam_mode = FIRST_PERSON;
        else
            cam_mode = THIRD_PERSON;
    }

    // Tecla L = ativa câmera livre
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
        cam_mode = FREE_CAM;

    // Teclas de Movimentação
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
        moveFoward = true;
    if (key == GLFW_KEY_W && action == GLFW_RELEASE)
        moveFoward = false;

    if (key == GLFW_KEY_A && action == GLFW_PRESS)
        if (cam_mode == THIRD_PERSON)
            rotateLeft = true;
        else
            moveLeft = true;
    if (key == GLFW_KEY_A && action == GLFW_RELEASE)
        if (cam_mode == THIRD_PERSON)
            rotateLeft = false;
        else
            moveLeft = false;

    if (key == GLFW_KEY_S && action == GLFW_PRESS)
        moveBackwards = true;
    if (key == GLFW_KEY_S && action == GLFW_RELEASE)
        moveBackwards = false;

    if (key == GLFW_KEY_D && action == GLFW_PRESS)
        if (cam_mode == THIRD_PERSON)
            rotateRight = true;
        else
            moveRight = true;
    if (key == GLFW_KEY_D && action == GLFW_RELEASE)
        if (cam_mode == THIRD_PERSON)
            rotateRight = false;
        else
            moveRight = false;

    // Ataque
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
        characters[active_character].attack();
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     World", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     Camera", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                   NDC", -1.0f, 1.0f-13*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-14*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

void BuildMeshes(int argc, char* argv[]){

    // Carregando modelo do cubo com arestas suaves - Terra, "montanhas"
    ObjModel boxmodel("../../data/box.obj");
    ComputeNormals(&boxmodel);
    BuildTrianglesAndAddToVirtualScene(&boxmodel);

    // Modelo de um cubo
    ObjModel cubemodel("../../data/cube.obj");
    ComputeNormals(&cubemodel);
    BuildTrianglesAndAddToVirtualScene(&cubemodel);

    ObjModel bodymodel("../../data/body.obj");
    ComputeNormals(&bodymodel);
    BuildTrianglesAndAddToVirtualScene(&bodymodel);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }
}

void CreateCharacters(glm::vec3 land_size) {

    glm::vec4 origin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    // Team 1
    Character spearman_1;
    spearman_1.init_attributes(SPEARMAN);
    spearman_1.set_team(1);
    spearman_1.position = glm::vec4(land_size.x / 4, 0.0f, -land_size.z / 4, 1.0f);
    spearman_1.position.y = scenary.heigth(spearman_1.position);
    spearman_1.camera.init_char(spearman_1.position, glm::vec4(spearman_1.position.x, spearman_1.position.y, -spearman_1.position.z, 1.0f));
    spearman_1.facing_vector = spearman_1.camera.view;
    characters.push_back(spearman_1);

    Character guardian_1;
    guardian_1.init_attributes(GUARDIAN);
    guardian_1.set_team(1);
    guardian_1.position = glm::vec4(0.0f, 0.0f, -land_size.z / 4, 1.0f);
    guardian_1.position.y = scenary.heigth(guardian_1.position);
    guardian_1.camera.init_char(guardian_1.position, glm::vec4(guardian_1.position.x, guardian_1.position.y, -guardian_1.position.z, 1.0f));
    guardian_1.facing_vector = guardian_1.camera.view;
    characters.push_back(guardian_1);

    Character archer_1;
    archer_1.init_attributes(ARCHER);
    archer_1.set_team(1);
    archer_1.position = glm::vec4(-land_size.x / 4, 0.0f, -land_size.z / 4, 1.0f);
    archer_1.position.y = scenary.heigth(archer_1.position);
    archer_1.camera.init_char(archer_1.position, glm::vec4(archer_1.position.x, archer_1.position.y, -archer_1.position.z, 1.0f));
    archer_1.facing_vector = archer_1.camera.view;
    characters.push_back(archer_1);

    // Team 2
    Character spearman_2;
    spearman_2.init_attributes(SPEARMAN);
    spearman_2.set_team(2);
    spearman_2.position = glm::vec4(land_size.x / 4, 0.0f, land_size.z / 4, 1.0f);
    spearman_2.position.y = scenary.heigth(spearman_2.position);
    spearman_2.camera.init_char(spearman_2.position, glm::vec4(spearman_2.position.x, spearman_2.position.y, -spearman_2.position.z, 1.0f));
    spearman_2.facing_vector = spearman_2.camera.view;
    characters.push_back(spearman_2);

    Character guardian_2;
    guardian_2.init_attributes(GUARDIAN);
    guardian_2.set_team(2);
    guardian_2.position = glm::vec4(0.0f, 0.0f, land_size.z / 4, 1.0f);
    guardian_2.position.y = scenary.heigth(guardian_2.position);
    guardian_2.camera.init_char(guardian_2.position, glm::vec4(guardian_2.position.x, guardian_2.position.y, -guardian_2.position.z, 1.0f));
    guardian_2.facing_vector = guardian_2.camera.view;
    characters.push_back(guardian_2);

    Character archer_2;
    archer_2.init_attributes(ARCHER);
    archer_2.set_team(2);
    archer_2.position = glm::vec4(-land_size.x / 4, 0.0f, land_size.z / 4, 1.0f);
    archer_2.position.y = scenary.heigth(archer_2.position);
    archer_2.camera.init_char(archer_2.position, glm::vec4(archer_2.position.x, archer_2.position.y, -archer_2.position.z, 1.0f));
    archer_2.facing_vector = archer_2.camera.view;
    characters.push_back(archer_2);
}

void Character::draw() {
    float angle = acos(dotproduct(facing_vector, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));

    if (facing_vector.x < 0.0)
        angle = -acos(dotproduct(facing_vector, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));

    glm::mat4 model = Matrix_Translate(position.x, position.y, position.z)
                    * Matrix_Scale(0.03f, 0.03f, 0.03f)
                    * Matrix_Rotate_Y(angle);
    glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(object_id_uniform, team + 1);
    DrawVirtualObject("Plane_Plane.003");
}

void Character::move()
{
    if (remaining_movement > 0){
        glm::vec4 future_place;
        future_place.x = position.x + camera.speed*facing_vector.x;
        future_place.y = position.y + camera.speed*facing_vector.y;  // não se mexe para cima ou para baixo
        future_place.z = position.z + camera.speed*facing_vector.z;
        future_place.w = 1.0;

        if (scenary.heigth(position) == scenary.heigth(future_place)){
            position.x += camera.speed*facing_vector.x;
            position.y += camera.speed*facing_vector.y;  // não se mexe para cima ou para baixo
            position.z += camera.speed*facing_vector.z;

            remaining_movement -= camera.speed;
            camera.position.x = position.x;
            camera.position.z = position.z;
        }
    }
}

void Character::moveFP(glm::vec4 direction)
{
    if (remaining_movement > 0){
        glm::vec4 future_place;
        future_place.x = position.x + camera.speed*facing_vector.x;
        future_place.y = position.y + camera.speed*facing_vector.y;  // não se mexe para cima ou para baixo
        future_place.z = position.z + camera.speed*facing_vector.z;
        future_place.w = 1.0;

        if (scenary.heigth(position) == scenary.heigth(future_place)){
            camera.move2D(normalize(direction));
            position.x = camera.position.x;
            position.z = camera.position.z;

            remaining_movement -= camera.speed;
        }
    }
}

void Character::attack()
{
    glm::vec4 vec;
    float distance;
    float angle;

    if (remaining_actions <= 0)
        return;

    for (int i = 0; i < characters.size(); i++)
        if (i != active_character && characters[i].team != team){
            vec = characters[i].position - position;
            distance = norm(vec);
            if(distance <= range){
                vec = normalize(vec);
                angle = dotproduct(facing_vector, vec);
                angle = acos(angle);

                if (angle <= 0.2618){
                    characters[i].take_damage(damage);
                    printf("character %d, hp %f\n", i, characters[i].current_hp);
                    remaining_actions--;
                    break;
                }
            }

        }
}

void DrawCharacters() {
    for (int i = 0; i < characters.size(); i++)
        characters[i].draw();
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :

// Funções de Classes
// Camera Livre
void Free_Camera::init(glm::vec4 pos, glm::vec4 origin)
{
    position = pos;//glm::vec4(0.0f, 2.0f, 2.0f, 1.0f);
    view = origin - position;
    view = view / norm(view);
    up = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    right = crossproduct(view, up);
    right = right / norm(right);
    v = crossproduct(view, right);

    phi = acos((dotproduct(view, up)) / (norm(view)*norm(up)));
    speed = 0.01f;
}

void Free_Camera::init_char(glm::vec4 pos, glm::vec4 look)
{
    glm::vec4 origin = glm::vec4(look.x, look.y + 0.28, look.z, 1.0);
    position = glm::vec4(pos.x, pos.y + 0.28, pos.z, 1.0f);
    view = origin - position;
    view = view / norm(view);
    up = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    right = crossproduct(view, up);
    right = right / norm(right);
    v = crossproduct(view, right);

    phi = acos((dotproduct(view, up)) / (norm(view)*norm(up)));
    speed = 0.01f;
}

void Free_Camera::move(glm::vec4 direction)
{
    position.x += speed*direction.x;
    position.y += speed*direction.y;
    position.z += speed*direction.z;
}

void Free_Camera::move2D(glm::vec4 direction)
{
    position.x += speed*direction.x;
    position.z += speed*direction.z;
}

void Free_Camera::rotate(glm::vec4 axis, float angle)
{
    view = Matrix_Rotate(angle, axis) * view;
    //Atualiza os vetores da câmera
    right = crossproduct(view, up);
    right = right / norm(right);

    v = crossproduct(right, view);
    v = v / norm(v);
}

void Free_Camera::look(float dtheta, float dphi)
{
    float phimax = 3.141f;
    float phimin = 0.01;

    phi += 0.005f*dphi;
    if (phi < phimin)
        phi = phimin;
    else if (phi > phimax)
        phi = phimax;
    else
        rotate(right, -0.005f*dphi);

    //Rotacionando view no eixo y (Olhar para os lados)
    rotate(up, -0.005f*dtheta);
}

// Camera Look-at-------------------------------
void Lookat_Camera::init(){
    theta = 0;
    phi = 3.1415/4;
    float r = distance;
    position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    // Computamos a posição da câmera utilizando coordenadas esféricas.
    position.x = r*cos(phi)*sin(theta);
    position.y = r*sin(phi);
    position.z = r*cos(phi)*cos(theta);
    // Outros vetores
    up = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    lookat = glm::vec4(0.0f,0.0f,0.0f,1.0f);
    view = lookat - position;
    //Parâmetros
    speed = 0.01f;
    nearplane = -0.1f;
    farplane = -10.0f;
}

void Lookat_Camera::move(glm::vec4 direction)
{
    lookat.x += speed*direction.x;
    lookat.z += speed*direction.z;

    update_camera();
}

void Lookat_Camera::look(float dtheta, float dphi)
{
    float phimax = 3.141592f/2;
    float phimin = 0.35;

    theta -= 0.01f*dtheta;
    phi += 0.01f*dphi;

    if (phi < phimin)
        phi = phimin;

    if (phi > phimax)
        phi = phimax;

    update_camera();
}

void Lookat_Camera::update_camera()
{
    float r = distance;
    // Computamos a posição da câmera utilizando coordenadas esféricas.
    position.x = r*cos(phi)*sin(theta) + lookat.x;
    position.y = r*sin(phi) + lookat.y + lookat.y;
    position.z = r*cos(phi)*cos(theta) + lookat.z;

    view = lookat - position;
}
void Lookat_Camera::set_nearplane(float near_value)
{
    nearplane = near_value;
}

void Lookat_Camera::set_farplane(float far_value)
{
    farplane = far_value;
}

// Scenary Functions
void Scenary::build(){
    // Definindo o tamanho do cenário
    land_size.x = (rand() % 75 + 25) * 0.04; // gera um número entre 25 e 100, depois escala por 0.04 para gerar um float de 1 a 4
    land_size.z = (rand() % 75 + 25) * 0.04;
    if ( land_size.z > 1.77*land_size.x )
        land_size.z = 1.77*land_size.x;
    else if ( land_size.z <  0.5625*land_size.x )
        land_size.z = 0.5625*land_size.x;

    if( land_size.x < 2)
        land_size.y = land_size.x;
    else if ( land_size.z < 2)
        land_size.y = land_size.z;
    else
        land_size.y = 2;

    // Definindo a quantidade de planaltos
    int plateau_qtd = rand() % 4;   // Gera um número aleatório de planalto entre 0 e 4
    // Laço que define os planaltos
    for(int i = 0; i < plateau_qtd; i++){
        Plateau plateau;

        // Define o tamanho
        if ( rand() % 2 == 0){
            plateau.scale.x = (rand() % 15 + 10)*0.01*land_size.x;
            plateau.scale.z = (rand() % 15 + 10)*0.01*land_size.z;
        }
        else{
            plateau.scale.x = (rand() % 5 + 20)*0.01*land_size.x;
            plateau.scale.z = (rand() % 15 + 10)*0.01*land_size.z;
        }

        plateau.scale.y = (rand() % 80 + 20)*0.01;
        if ( plateau.scale.y > plateau.scale.x)
            plateau.scale.y = plateau.scale.x;
        if ( plateau.scale.y > plateau.scale.z)
            plateau.scale.y = plateau.scale.z;

        // Define a posição
        // De acordo com o valor de i, o planalto é mandado para um determinado quadrante
        if ( i == 0 ){ // Quadrante negativo
            plateau.position.x = (plateau.scale.x - land_size.x) * 0.5 * (rand() % 80 + 20) * 0.01;
            plateau.position.z = (plateau.scale.z - land_size.z) * 0.5 * (rand() % 80 + 20) * 0.01;
        }
        else if ( i == 1 ){
            plateau.position.x = (land_size.x - plateau.scale.x) * 0.5 * (rand() % 80 + 20) * 0.01;
            plateau.position.z = (plateau.scale.z - land_size.z) * 0.5 * (rand() % 80 + 20) * 0.01;
        }
        else if ( i == 2 ){
            plateau.position.x = (plateau.scale.x - land_size.x) * 0.5 * (rand() % 80 + 20) * 0.01;
            plateau.position.z = (land_size.z - plateau.scale.z) * 0.5 * (rand() % 80 + 20) * 0.01;
        }
        else{
            plateau.position.x = (land_size.x - plateau.scale.x) * 0.5 * (rand() % 80 + 20) * 0.01;
            plateau.position.z = (land_size.z - plateau.scale.z) * 0.5 * (rand() % 80 + 20) * 0.01;
        }

        plateau.position.y = plateau.scale.y * 0.5 * 0.9;

        plateaus.push_back(plateau);
    }

}

void Scenary::draw(){
    glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

    //Desenhamos a terra
    model = Matrix_Translate(0.0f, -land_size.y/2, 0.0f)
          * Matrix_Scale(land_size.x, land_size.y, land_size.z);
    glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(object_id_uniform, LAND);
    DrawVirtualObject("Box");

    //Desenhamos a água
    model = Matrix_Translate(0.0f, -land_size.y*1.1f, 0.0f)
          * Matrix_Scale(land_size.x*2, land_size.y*2, land_size.z*2);
    glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(object_id_uniform, WATER);
    DrawVirtualObject("cube");

    // TODO Desenhamos os planaltos
    for (int i = 0; i < plateaus.size(); i++)
    {
        model = Matrix_Translate(plateaus[i].position.x,
                                 plateaus[i].position.y,
                                 plateaus[i].position.z)
              * Matrix_Scale(plateaus[i].scale.x,
                             plateaus[i].scale.y,
                             plateaus[i].scale.z);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, LAND);
        DrawVirtualObject("Box");
    }
}

float Scenary::heigth(glm::vec4 dot)
{
    float heigth = 0.0;
    float x_min, x_max, z_min, z_max;

    // Testa se está em cima da terra primeiro
    x_min = -(land_size.x / 2) + 0.1;
    x_max = -x_min;
    z_min = -(land_size.z / 2) + 0.1;
    z_max = -z_min;

    if (dot.x < x_min)
        heigth = -1;
    else if (dot.x > x_max)
        heigth = -1;
    else if (dot.z < z_min)
        heigth = -1;
    else if (dot.z > z_max)
        heigth = -1;
    else
        heigth = 0;
    // Se estiver fora da terra, não precisa testar os planaltos
    if ( heigth == -1)
        return heigth;

    for(int i = 0; i < plateaus.size(); i++){
        x_min = (plateaus[i].position.x - plateaus[i].scale.x / 2);
        x_max = (plateaus[i].position.x + plateaus[i].scale.x / 2);
        z_min = (plateaus[i].position.z - plateaus[i].scale.z / 2);
        z_max = (plateaus[i].position.z + plateaus[i].scale.z / 2);

        if (dot.x > x_min && dot.x < x_max && dot.z > z_min && dot.z < z_max)
            heigth = plateaus[i].position.y + plateaus[i].scale.y / 2;

    }

    return heigth;
}

void Character::init_attributes(int type){
    role = type;
    switch(type){
        case SPEARMAN:
            max_hp = 100;
            current_hp = max_hp;
            max_movement = 2;
            remaining_movement = max_movement;
            max_actions = 1;
            remaining_actions = max_actions;
            range = 0.3;
            damage = 50;
            break;
        case GUARDIAN:
            max_hp = 200;
            current_hp = max_hp;
            max_movement = 1;
            remaining_movement = max_movement;
            max_actions = 1;
            remaining_actions = max_actions;
            range = 0.2;
            damage = 35;
            break;
        case ARCHER:
            max_hp = 70;
            current_hp = max_hp;
            max_movement = 2.5;
            remaining_movement = max_movement;
            max_actions = 1;
            remaining_actions = max_actions;
            range = 5;
            damage = 50;
            break;
    }
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slide 100 do documento "Aula_20_e_21_Mapeamento_de_Texturas.pdf"
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Parâmetros de amostragem da textura. Falaremos sobre eles em uma próxima aula.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

glm::vec4 normalize(glm::vec4 aim_vector)
{
    float x = aim_vector.x;
    float y = aim_vector.y;
    float z = aim_vector.z;
    float norm = sqrt(x*x + y*y + z*z);

    return aim_vector / norm;
}

void PassTurn()
{
    characters[active_character].end_turn();
    active_character++;
    if (active_character > characters.size() - 1)
        active_character = 0;

    lookat_camera.lookat = characters[active_character].position;
    lookat_camera.update_camera();
}
