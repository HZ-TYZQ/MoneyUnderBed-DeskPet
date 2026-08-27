# 可移植发行物的统一安装布局。Actions 只在此布局之上收集 Qt／系统运行库，
# 不各自维护一套手工复制清单。

if(WIN32)
    set(MUB_RUNTIME_INSTALL_DIR ".")
elseif(LINUX)
    # AppImage 的固定 FHS 布局。安装前缀直接指向 AppDir。
    set(MUB_RUNTIME_INSTALL_DIR "usr/bin")
else()
    message(FATAL_ERROR "Portable packaging is only supported on Linux and Windows")
endif()

install(TARGETS money-under-bed-deskpet
    RUNTIME DESTINATION "${MUB_RUNTIME_INSTALL_DIR}"
)

# 角色素材按其单独授权作为原始只读数据文件发行，不进入 Qt 资源系统。
# 它们紧邻可执行文件，确保 Windows ZIP 与 AppImage 使用相同定位规则。
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/character"
    DESTINATION "${MUB_RUNTIME_INSTALL_DIR}/assets"
    FILES_MATCHING PATTERN "*.png"
)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/face"
    DESTINATION "${MUB_RUNTIME_INSTALL_DIR}/assets"
    FILES_MATCHING PATTERN "*.png"
)

# 法务文件在两种发行物中使用完全相同的相对路径。
install(FILES "${CMAKE_SOURCE_DIR}/LICENSE"
    DESTINATION "."
)
# Qt 使用 GPL-3.0-only 路径。项目根 LICENSE 与 Qt 随源码提供的 GPLv3
# 正文仅 URL 协议不同，均是完整 GPLv3；单独安装一份可让清单路径稳定。
install(FILES "${CMAKE_SOURCE_DIR}/LICENSE"
    DESTINATION "licenses"
    RENAME "qt-gpl-3.0-only.txt"
)
install(FILES "${CMAKE_SOURCE_DIR}/third_party/ark-pixel-font/OFL.txt"
    DESTINATION "licenses"
)
install(FILES "${CMAKE_SOURCE_DIR}/third_party/ark-pixel-font/README.md"
    DESTINATION "licenses"
    RENAME "ark-pixel-font.md"
)
install(FILES "${CMAKE_SOURCE_DIR}/assets/LICENSE.md"
    DESTINATION "licenses"
    RENAME "assets.md"
)
install(FILES "${CMAKE_SOURCE_DIR}/assets/MANIFEST.md"
    DESTINATION "licenses"
    RENAME "assets-manifest.md"
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES.md"
    DESTINATION "licenses"
    RENAME "README.md"
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/licenses/qt-source.md"
    DESTINATION "licenses"
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/licenses/qt-modules-6.11.2.spdx"
    DESTINATION "licenses"
)

if(LINUX)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/licenses/appimage-runtime.txt"
        DESTINATION "licenses"
    )
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/licenses/appimage-tooling.txt"
        DESTINATION "licenses"
    )
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/linux/io.github.hz_tyzq.MoneyUnderBedDeskPet.desktop"
        DESTINATION "usr/share/applications"
    )
    # AppImage 工具只接受标准正方形图标。该文件是 natural.png 居中放入
    # 160x160 透明画布的机械衍生图，不缩放原像素；见 assets/MANIFEST.md。
    install(FILES "${CMAKE_SOURCE_DIR}/assets/icon/appimage.png"
        DESTINATION "usr/share/icons/hicolor/160x160/apps"
        RENAME "io.github.hz_tyzq.MoneyUnderBedDeskPet.png"
    )
elseif(WIN32)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/licenses/msvc-runtime.md"
        DESTINATION "licenses"
    )
endif()
