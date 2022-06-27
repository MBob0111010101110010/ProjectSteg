#include "stdaftx.h"
#include "CppUnitTest.h"
#include "../ProjectSteg/ProjectSteg"
using namespace Miscrosoft::VisualStudioCode::CppUnitTestFramework

namespace ProjectSteg

     TEST_CLASS(Unittest1)
     {
        public:

        TEST_METHOD(TestMethod1)
        {
            bool is_valid_image_path(char *path) {
                int l = strlen(path);
                return strcmp(path + l - 4, ".bmp") == 0 ||
                 strcmp(path + l - 4, ".png") == 0;

        }
     }