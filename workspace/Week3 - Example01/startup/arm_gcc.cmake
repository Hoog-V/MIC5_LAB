set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)
if(${CMAKE_VERSION} VERSION_LESS "3.16.0")
    message(WARNING "Current CMake version is ${CMAKE_VERSION}. KL25Z-cmake requires CMake 3.16 or greater")

endif()

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")


set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(MCPU cortex-m0plus)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)


add_compile_options(-mcpu=${MCPU} -mthumb -mthumb-interwork)

add_definitions(-DCPU_MKL25Z128VLK4 
                -DFRDM_KL25Z 
                -DFREEDOM
                -DDEBUG
                -D__USE_CMSIS
                -DCLOCK_SETUP=1
                -MMD
                -MP
                -fno-common 
                -g3 
                -gdwarf-4 
                -Wall 
                -fmessage-length=0 
                -fno-builtin 
                -ffunction-sections 
                -fdata-sections 
                -fmerge-constants
                -mapcs)

set(LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/linker_script.ld)
add_link_options(-lm -Wl,-gc-sections,--print-memory-usage,--no-warn-rwx-segments,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map --specs=nano.specs -Xlinker -static -mthumb)
add_link_options(-T ${LINKER_SCRIPT} -static)
