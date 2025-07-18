cmake_minimum_required(VERSION 3.5)
project(ReconhecimentoFacial2)

# Recomendo usar C++17 para evitar problemas de compatibilidade
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontre o OpenCV instalado no sistema
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui objdetect)

# Se usar dlib (comente se não for usar)
# find_package(dlib REQUIRED)
# include_directories(${dlib_INCLUDE_DIRS})

# Inclua diretórios comuns
include_directories(${OpenCV_INCLUDE_DIRS})

# Executável para cadastro de faces
add_executable(enroll src/enroll.cpp)
target_link_libraries(enroll ${OpenCV_LIBS})

# Executável para reconhecimento/registro de presença
add_executable(attend src/attend.cpp)
target_link_libraries(attend ${OpenCV_LIBS})

# Se quiser adicionar outras fontes (.cpp), basta seguir o mesmo padrão acima
# Exemplo:
# add_executable(outro src/outro.cpp)
# target_link_libraries(outro ${OpenCV_LIBS})
