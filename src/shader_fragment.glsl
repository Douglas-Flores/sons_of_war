#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 position_model;
in vec4 normal;

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

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;

in vec2 texcoords;
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec3 color;
vec3 color0;
vec3 color1;

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
    vec4 l = normalize(vec4(1.0f, 1.0f, 0.5f, 0.0f));

    // Vetor que define o sentido da reflexão especular ideal.
    vec4 r = -l + 2*n*dot(n,l);

    // Parâmetros que definem as propriedades espectrais da superfície
    vec3 Kd; // Refletância difusa
    vec3 Ks; // Refletância especular
    vec3 Ka; // Refletância ambiente
    float q; // Expoente especular para o modelo de iluminação de Phong

    float U = 0.0;
    float V = 0.0;

    if ( object_id == LAND )
    {
        /*float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_world.x - minx)/(maxx - minx);
        V = (position_world.y - miny)/(maxy - miny);*/
        Kd = vec3(0.1,0.9,0.1);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.1,0.5,0.3);
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
    }
    else if ( object_id == CHAR_TEAM_1)
    {
        Kd = vec3(0.9,0.1,0.1);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.9,0.1,0.1);
        q = 0.0;
    }
    else if ( object_id == CHAR_TEAM_2)
    {
        Kd = vec3(0.2,0.2,1.0);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.2,0.2,1.0);
        q = 0.0;
    }
    else // Objeto desconhecido = preto
    {
        Kd = vec3(0.2,0.45,0.1);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.0,0.0,0.0);
        q = 20.0;
    }



    // Atributos da Fonte de Iluminação SPOTLIGHT
    vec3 I = vec3(1.0,1.0,1.0); // Espectro da fonte de iluminação

    // Espectro da luz ambiente
    vec3 Ia = vec3(0.1,0.1,0.1);

    // Termo difuso utilizando a lei dos cossenos de Lambert
    vec3 lambert_diffuse_term = Kd*I*max(0,dot(n,l));

    // Termo ambiente
    vec3 ambient_term = Ka*Ia;

    // Termo especular utilizando o modelo de iluminação de Phong
    vec3 phong_specular_term  = Ks*I*max(0,pow(dot(r,v),q));

        // Obtemos a refletância difusa a partir da leitura das texturas.
    vec3 Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    vec3 Kd1 = texture(TextureImage1, vec2(U,V)).rgb;

    float lambert0 = max(0,dot(n,l));
    float lambert1 = max(0,dot(n,l));

    color0 = Kd0 * (lambert0 + 0.01);
    color1 = Kd1 * (lambert1 + 0.01);

    // Cor final do fragmento calculada com uma combinação dos termos difuso,
    // especular, e ambiente. Veja slide 133 do documento "Aula_17_e_18_Modelos_de_Iluminacao.pdf".
    color = lambert_diffuse_term + ambient_term + phong_specular_term;

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color = pow(color, vec3(1.0,1.0,1.0)/2.2);

    /*if(object_id == LAND)
    {
         color = pow(color0, vec3(1.0,1.0,1.0)/2.2);;
    }

    else */if(object_id == WATER)
    {
         color = pow(color1, vec3(1.0,1.0,1.0)/4.2);
    }

}

