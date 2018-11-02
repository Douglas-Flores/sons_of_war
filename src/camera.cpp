// Classes de Camera

#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <string>
#include <limits>
#include <fstream>
#include <sstream>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

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

    void init();
    //void move(glm::vec4 direction);
    //void rotate(glm::vec4 axis, float angle);
    //void look(float dtheta, float dphi);*/
};

void Free_Camera::init()
{
    position = glm::vec4(0.0f, 2.0f, 2.0f, 1.0f);
    view = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) - position;
    view = view / norm(view);
    up = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    right = crossproduct(view, up);
    right = right / norm(right);
    v = crossproduct(view, right);

    phi = acos((dotproduct(view, up)) / (norm(view)*norm(up)));
    speed = 0.01f;
}

/*void Free_Camera::move(glm::vec4 direction)
{
    /*position.x += speed*direction.x;
    position.y += speed*direction.y;
    position.z += speed*direction.z;
}

void Free_Camera::rotate(glm::vec4 axis, float angle)
{
    /*view = Matrix_Rotate(angle, axis) * view;
    //Atualiza os vetores da câmera
    right = crossproduct(view, up);
    right = right / norm(right);

    v = crossproduct(right, view);
    v = v / norm(v);
}

/*void Free_Camera::look(float dtheta, float dphi)
{
    /*float phimax = 3.141f;
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
}*/
