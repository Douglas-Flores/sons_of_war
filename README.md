# Sons of War
Jogo eletrônico de estratégia desenvolvido como trabalho para a disciplina de Fundamento de Computação Gráfica - UFRGS

# Rodando o Projeto

## Windows
Para compilar e executar este projeto no Windows, baixe a IDE Code::Blocks em http://codeblocks.org/ e abra o arquivo "Sons_of_War.cbp".

## Linux
Para compilar e executar este projeto no Linux, primeiro você precisa instalar as bibliotecas necessárias. Para tanto, execute o comando abaixo em um terminal.
Esse é normalmente suficiente em uma instalação de Linux Ubuntu:

#+begin_src shell :results output
sudo apt-get install build-essential make libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxcb1-dev libxext-dev libxrender-dev libxfixes-dev libxau-dev libxdmcp-dev
#+end_src

Após a instalação das bibliotecas acima, você possui duas opções para compilação: utilizar Code::Blocks ou Makefile.

### Linux com Code::Blocks
Instale a IDE Code::Blocks (versão Linux em http://codeblocks.org/), abra o arquivo "Sons_of_War.cbp", e modifique o "Build target" de "Debug" para "Linux".

### Linux com Makefile
Abra um terminal, navegue até a pasta "Sons_of_War_Codigo_Fonte", e execute o comando "make" para compilar. Para executar o código compilado, execute o comando "make run".

## macOS
Para compilar e executar esse projeto no macOS, primeiro você precisa instalar o HOMEBREW, um gerenciador de pacotes para facilitar a instação de bibliotecas. O HOMEBREW pode ser instalado com o seguinte comando no terminal:

#+begin_src shell :results output
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
#+end_src

Após a instalação do HOMEBREW, a biblioteca GLFW deve ser instalada. Isso pode ser feito pelo terminal com o comando:
#+begin_src shell :results output
brew install glfw
#+end_src

### macOS com Makefile
Abra um terminal, navegue até a pasta "Sons_of_War_Codigo_Fonte", e execute o comando "make -f Makefile.macOS" para compilar. Para executar o código compilado, execute o comando "make -f Makefile.macOS run".

Observação: a versão atual da IDE Code::Blocks é bastante desatualizada pra o macOS. A nota oficial dos desenvolvedores é: "Code::Blocks 17.12 for Mac is currently not available due to the lack of Mac developers, or developers that own a Mac. We could use an extra Mac developer (or two) to work on Mac compatibility issues."

### Soluções de Problemas
Caso você tenha problemas em executar o código deste projeto, tente atualizar o driver da sua placa de vídeo.

