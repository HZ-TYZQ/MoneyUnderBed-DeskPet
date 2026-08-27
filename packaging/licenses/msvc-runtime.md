# Microsoft Visual C++ Runtime 2015–2022

The Windows portable package carries unmodified release CRT DLLs copied from
the GitHub Actions `windows-2022` runner's `%VCToolsRedistDir%` directory. They
are installed next to the application for app-local deployment; debug and
`debug_nonredist` files are rejected by the packaging workflow.

Microsoft's Visual Studio 2022 redistribution list permits a validly licensed
Visual Studio user to distribute unmodified files from the Visual Studio
`VC\Redist` tree with their program, subject to the applicable Microsoft
Software License Terms:

* Redistribution list:
  https://learn.microsoft.com/visualstudio/releases/2022/redistribution
* Visual C++ redistribution and licensing guidance:
  https://learn.microsoft.com/cpp/windows/redistributing-visual-cpp-files
* Microsoft Visual C++ Runtime 2015–2022 Software terms:
  https://visualstudio.microsoft.com/license-terms/

These Microsoft binaries are not part of MoneyUnderBed DeskPet and are not
licensed under GPL-3.0-or-later. They may not be modified or redistributed
except as permitted by Microsoft's terms.
