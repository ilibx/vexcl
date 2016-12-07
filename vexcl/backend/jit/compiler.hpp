#ifndef VEXCL_BACKEND_JIT_COMPILER_HPP
#define VEXCL_BACKEND_JIT_COMPILER_HPP

/*
The MIT License

Copyright (c) 2012-2016 Denis Demidov <dennis.demidov@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/**
 * \file   vexcl/backend/jit/compiler.hpp
 * \author Denis Demidov <dennis.demidov@gmail.com>
 * \brief  JIT compile support.
 */

#include <string>
#include <sstream>
#include <fstream>
#include <boost/dll/shared_library.hpp>

#include <vexcl/backend/common.hpp>
#include <vexcl/detail/backtrace.hpp>

#ifndef VEXCL_JIT_COMPILER
#  define VEXCL_JIT_COMPILER "g++"
#endif

#ifndef VEXCL_JIT_COMPILER_OPTIONS
#  ifdef NDEBUG
#    define VEXCL_JIT_COMPILER_OPTIONS "-O3 -fPIC -shared -fopenmp"
#  else
#    define VEXCL_JIT_COMPILER_OPTIONS "-g -fPIC -shared -fopenmp"
#  endif
#endif

namespace vex {
namespace backend {
namespace jit {

/// Compile and load a program from source string.
inline vex::backend::program build_sources(const command_queue &q,
        const std::string &source, const std::string &options = ""
        )
{
#ifndef VEXCL_SHOW_KERNELS
    if (getenv("VEXCL_SHOW_KERNELS"))
#endif
        std::cout << source << std::endl;

    std::string compile_options = options + " " + get_compile_options(q);

    sha1_hasher sha1;
    sha1.process(source)
        .process(compile_options)
        .process(VEXCL_JIT_COMPILER)
        .process(VEXCL_JIT_COMPILER_OPTIONS);

    std::string hash = static_cast<std::string>(sha1);

    // Write source to a .cpp file
    std::string basename = program_binaries_path(hash, true) + "kernel";
    std::string sofile   = basename + ".so";

    if ( !boost::filesystem::exists(sofile) ) {
        std::string cppfile = basename + ".cpp";

        {
            std::ofstream f(cppfile);
            f << source;
        }

        // Compile the source.
        std::ostringstream cmdline;
        cmdline << VEXCL_JIT_COMPILER << " -o " << sofile << " " << cppfile << " "
                << VEXCL_JIT_COMPILER_OPTIONS << " " << compile_options;

        if (0 != system(cmdline.str().c_str()) ) {
#ifndef VEXCL_SHOW_KERNELS
            std::cerr << source << std::endl;
#endif

            vex::detail::print_backtrace();
            throw std::runtime_error("Kernel compilation failed");
        }
    }

    // Load the compiled shared library.
    return boost::dll::shared_library(sofile);
}

} // namespace cuda
} // namespace backend
} // namespace vex

#endif
