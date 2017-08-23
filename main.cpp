// Copyright(c) 2015-2016, NVIDIA CORPORATION. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cassert>
//#include <iterator>
//#include <list>
//#include <exception>

#include "vkspec.h"
#include "rust_generator.h"
#include "cpp_dispatch_tables.h"

int main(int argc, char **argv)
{
  // TODO:
  // Extract translators to themselves
  // Probably useful with something like factory that can run a certain generator
	try {
		std::string filename = (argc == 1) ? VK_SPEC : argv[1];

        {
          RustTranslator translator;
          vkspec::Registry reg(&translator);
          reg.parse(filename);
          vkspec::Feature* feature = reg.build_feature("vulkan");

          std::string out = std::string(VULKAN_DIR) + "/vulkan.rs";
          std::cout << "Writing vulkan.rs to " << out << std::endl;

          RustGenerator generator(out, reg.license(), feature->major(), feature->minor(), feature->patch());
          feature->generate(&generator);
        }

        {
          CppTranslator translator;
          vkspec::Registry reg(&translator);
          reg.parse(filename);
          vkspec::Feature* feature = reg.build_feature("vulkan");

          CppDispatchTableGenerator generator(VULKAN_DIR, reg.license(), feature->major(), feature->minor(), feature->patch());
          feature->generate(&generator);
        }
	}
	catch (std::exception const& e)
	{
		std::cout << "caught exception: " << e.what() << std::endl;
		return -1;
	}
	catch (...)
	{
		std::cout << "caught unknown exception" << std::endl;
		return -1;
	}
}
