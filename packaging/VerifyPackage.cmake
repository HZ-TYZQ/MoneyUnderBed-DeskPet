if(NOT DEFINED PACKAGE_ROOT OR PACKAGE_ROOT STREQUAL "")
    message(FATAL_ERROR "PACKAGE_ROOT is required")
endif()
cmake_path(ABSOLUTE_PATH PACKAGE_ROOT NORMALIZE OUTPUT_VARIABLE root)
if(NOT IS_DIRECTORY "${root}")
    message(FATAL_ERROR "Package root does not exist: ${root}")
endif()

set(required_files
    "LICENSE"
    "licenses/OFL.txt"
    "licenses/ark-pixel-font.md"
    "licenses/assets.md"
    "licenses/assets-manifest.md"
    "licenses/README.md"
    "licenses/qt-source.md"
    "licenses/qt-gpl-3.0-only.txt"
    "licenses/qt-modules-6.11.2.spdx"
    "BUILD_INFO.txt"
)
if(DEFINED PACKAGE_EXECUTABLE AND NOT PACKAGE_EXECUTABLE STREQUAL "")
    list(APPEND required_files "${PACKAGE_EXECUTABLE}")
endif()
if(DEFINED PACKAGE_REQUIRED_FILES AND NOT PACKAGE_REQUIRED_FILES STREQUAL "")
    list(APPEND required_files ${PACKAGE_REQUIRED_FILES})
endif()

foreach(relative IN LISTS required_files)
    set(path "${root}/${relative}")
    if(NOT EXISTS "${path}" OR IS_DIRECTORY "${path}")
        message(FATAL_ERROR "Required package file is missing: ${relative}")
    endif()
    file(SIZE "${path}" size)
    if(size EQUAL 0)
        message(FATAL_ERROR "Required package file is empty: ${relative}")
    endif()
endforeach()

if(DEFINED PACKAGE_FORBIDDEN_PATHS AND NOT PACKAGE_FORBIDDEN_PATHS STREQUAL "")
    foreach(relative IN LISTS PACKAGE_FORBIDDEN_PATHS)
        if(EXISTS "${root}/${relative}" OR IS_SYMLINK "${root}/${relative}")
            message(FATAL_ERROR "Forbidden package path is present: ${relative}")
        endif()
    endforeach()
endif()

file(READ "${root}/licenses/qt-modules-6.11.2.spdx" qt_sbom)
foreach(package_id IN ITEMS QtBase)
    if(NOT qt_sbom MATCHES "SPDXID:[ \t]+SPDXRef-Package-${package_id}")
        message(FATAL_ERROR "Qt SBOM does not identify ${package_id} 6.11.2")
    endif()
endforeach()

# 角色素材必须作为外部文件紧邻可执行文件。固定清单既能发现漏包，
# 也让两种发行格式对单独授权素材采用完全相同的结构。
if(DEFINED PACKAGE_EXECUTABLE AND NOT PACKAGE_EXECUTABLE STREQUAL "")
    cmake_path(GET PACKAGE_EXECUTABLE PARENT_PATH executable_dir)
    set(asset_dir "${root}/${executable_dir}/assets")
    set(character_assets
        idle-up-left.png idle-down-left.png idle-up-right.png idle-down-right.png
        run-up-left.png run-down-left.png run-up-right.png run-down-right.png
        icecream-drop.png icecream-drop-still.png icecream-eat.png
    )
    set(face_assets
        natural.png natural-lower-eyes-brow.png
        serious-eyes-open.png serious-eyes-closed.png impatient.png happy.png
        proud.png proud-catmouth.png proud-thumb.png panic.png shadow.png
    )
    foreach(file IN LISTS character_assets)
        list(APPEND external_character_files "${asset_dir}/character/${file}")
    endforeach()
    foreach(file IN LISTS face_assets)
        list(APPEND external_character_files "${asset_dir}/face/${file}")
    endforeach()
    foreach(path IN LISTS external_character_files)
        if(NOT EXISTS "${path}" OR IS_DIRECTORY "${path}")
            message(FATAL_ERROR "External character asset is missing: ${path}")
        endif()
        file(SIZE "${path}" size)
        if(size EQUAL 0)
            message(FATAL_ERROR "External character asset is empty: ${path}")
        endif()
    endforeach()
endif()

# Ark Pixel 已作为 Qt 资源嵌入产品可执行文件。目录里出现 TTF 就代表重复打包。
file(GLOB_RECURSE external_fonts LIST_DIRECTORIES false
    "${root}/*.ttf" "${root}/*.TTF")
if(external_fonts)
    message(FATAL_ERROR "External TTF files must not be packaged: ${external_fonts}")
endif()

file(GLOB_RECURSE leaked_logs LIST_DIRECTORIES false
    "${root}/*.log" "${root}/*.LOG")
if(leaked_logs)
    message(FATAL_ERROR "Verification logs leaked into package: ${leaked_logs}")
endif()

if(EXISTS "${root}/vc_redist.x64.exe")
    message(FATAL_ERROR "vc_redist.x64.exe is an installer and must not be in the portable ZIP")
endif()

# Linux runtime provenance is generated from the populated AppDir.  A second
# linuxdeploy pass creates the AppImage, so verify the final extracted image
# against that manifest and reject any library that appeared after collection.
if(EXISTS "${root}/licenses/linux-runtime.tsv")
    file(READ "${root}/licenses/linux-runtime.tsv" runtime_manifest)
    file(GLOB bundled_libraries LIST_DIRECTORIES false "${root}/usr/lib/*.so*")
    foreach(path IN LISTS bundled_libraries)
        cmake_path(RELATIVE_PATH path BASE_DIRECTORY "${root}" OUTPUT_VARIABLE relative)
        string(FIND "${runtime_manifest}" "${relative}\t" record_offset)
        if(record_offset EQUAL -1)
            message(FATAL_ERROR
                "Bundled Linux library has no provenance record: ${relative}")
        endif()
    endforeach()
endif()

message(STATUS "Portable package layout verified: ${root}")
