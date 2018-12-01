#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


// Identificador que define qual objeto está sendo desenhado no momento
#define LAND        0
#define WATER       1
#define CHAR_TEAM_1 2
#define CHAR_TEAM_2 3
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
uniform sampler2D TextureImage5;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;
vec4 color0;

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0f, 1.0f, 0.0f, 0.0f));

    // Vetor que define o sentido da reflexão especular ideal.
    vec4 r = -l + 2*n*dot(n,l);

    // Vetor que define o meio do caminho entre a luz e a câmera
    vec4 h = normalize(v + l);

    // Parâmetros que definem as propriedades espectrais da superfície
    vec4 Kd; // Refletância difusa
    vec4 Ks; // Refletância especular
    vec4 Ka; // Refletância ambiente
    float q; // Expoente especular para o modelo de iluminação de Phong

    float U = 0.0;
    float V = 0.0;

    if ( object_id == LAND )
    {
        // limites
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

        float theta;
        float phi;
        float phi2;

        // Vetor p partindo do centro até o ponto
        vec4 p_model = position_model - vec4(0.0f, 0.0f, 0.0f, 1.0f);
        p_model = normalize(p_model);
        // Vetor unitário do eixo y
        vec4 up = vec4(0.0f, 1.0f, 0.0f, 0.0f);
        // Vetor unitário eixo x
        vec4 vecX = vec4(1.0f, 0.0f, 0.0f, 0.0f);
        // Projeção do vetor p_model no plano xz
        vec4 p_modelXZ = vec4(p_model.x, 0.0f, p_model.z, p_model.w);
        p_modelXZ = normalize(p_modelXZ);
        // Projeção do vetor p_model no plano xy
        vec4 p_modelXY = vec4(p_model.x, p_model.y, 0.0f, p_model.w);
        p_modelXY = normalize(p_modelXY);
        // Projeção do vetor p_model no plano yz
        vec4 p_modelYZ = vec4(0.0f, p_model.y, p_model.z, p_model.w);
        p_modelYZ = normalize(p_modelYZ);
        // Ângulos
        phi = dot(p_modelXY, up);
        phi = acos(phi);
        phi2 = dot(p_modelYZ, up);
        phi2 = acos(phi2);
        theta = dot(p_modelXZ, vecX);
        theta = acos(theta);
        // Determinando a face
        // Top
        if (phi <= M_PI / 4 && phi2 <= M_PI / 4)
        {
            U = (position_model.x - minx)/(maxx - minx);
            V = (position_model.z - minz)/(maxz - minz);
            Kd = texture(TextureImage2, vec2(U,V)).rgba;
        }
        // Front
        else if (theta <= M_PI / 4)
        {
            U = (position_model.y - miny)/(maxy - miny);
            V = (position_model.z - minz)/(maxz - minz);
            Kd = texture(TextureImage4, vec2(U,V)).rgba;
        }
        // Sides
        else if (theta > M_PI / 4 && theta <= 3 * M_PI / 4)
        {
            U = (position_model.x - minx)/(maxx - minx);
            V = (position_model.y - miny)/(maxy - miny);
            Kd = texture(TextureImage4, vec2(U,V)).rgba;
        }
        // Back
        else if (theta > 3 * M_PI / 4 && theta <= M_PI)
        {
            U = (position_model.y - miny)/(maxy - miny);
            V = (position_model.z - minz)/(maxz - minz);
            Kd = texture(TextureImage4, vec2(U,V)).rgba;
        }
        else
        {
            U = (theta + M_PI) / (2*M_PI);
            V = (phi + M_PI_2) / M_PI;
            Kd = texture(TextureImage2, vec2(U,V)).rgba;
        }
        Ks = vec4(0.0, 0.0, 0.0, 1.0);
        Ka = vec4(0.0, 0.5, 0.0, 1.0);
        q = 0.0;
    }
    else if ( object_id == WATER )
    {
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_world.x - minx)/(maxx - minx);
        V = (position_world.z - minz)/(maxz - minz);

        /*Kd = texture(TextureImage3, vec2(U,V)).rgb;
        n.x = (Kd.x);
        n.y = (Kd.y);
        n.z = (Kd.z);
        n.w = 0.0f;*/
        //n = normalize(n);
        Kd = vec4(0.05f, 0.45f, 0.8f, 0.01f);
        Ks = vec4(0.5, 0.5, 0.5, 1.0f);
        Ka = vec4(0.05, 0.45, 0.8, 1.0);
        q = 512.0;
    }
    else if ( object_id == CHAR_TEAM_1)
    {
        Kd = vec4(0.9,0.1,0.1,1.0);
        Ks = vec4(0.0,0.0,0.0,1.0);
        Ka = vec4(0.9,0.1,0.1,1.0);
        q = 0.0;
    }
    else if ( object_id == CHAR_TEAM_2)
    {
        Kd = vec4(0.2,0.2,1.0,1.0);
        Ks = vec4(0.0,0.0,0.0,1.0);
        Ka = vec4(0.2,0.2,1.0,1.0);
        q = 0.0;
    }
    else // Objeto desconhecido = preto
    {
        Kd = vec4(0.0,0.0,0.0,1.0);
        Ks = vec4(0.0,0.0,0.0,1.0);
        Ka = vec4(0.0,0.0,0.0,1.0);
        q = 20.0;
    }

    vec4 I = vec4(1.0,1.0,0.8,1.0); // Espectro da fonte de iluminação

    // Espectro da luz ambiente
    vec4 Ia = vec4(0.1,0.1,0.1,1.0);

    // Termo difuso utilizando a lei dos cossenos de Lambert
    vec4 lambert_diffuse_term = Kd*I*max(0,dot(n,l));

    // Termo ambiente
    vec4 ambient_term = Ka*Ia;

    // Termo especular utilizando o modelo de iluminação de Phong
    vec4 phong_specular_term  = Ks*I*max(0,pow(dot(n,h),q));

    float lambert0 = max(0,dot(n,l));

    color0 = Kd * (lambert0 + 0.01);

    // Cor final do fragmento calculada com uma combinação dos termos difuso,
    // especular, e ambiente. Veja slide 133 do documento "Aula_17_e_18_Modelos_de_Iluminacao.pdf".
    color = lambert_diffuse_term + ambient_term + phong_specular_term;

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color = pow(color, vec4(1.0,1.0,1.0,1.0)/2.2);

    if(object_id == LAND)
         color = pow(color0, vec4(1.0,1.0,1.0,1.0)/4.2);

    else if(object_id == WATER)
    {
         color = pow(color, vec4(1.0,1.0,1.0,1.0)/1.2);
         color.a = 0.1;
    }

}

