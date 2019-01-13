
# Copyright(c) 2015 - 2019 Denis Blank <denis.blank at outlook dot com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set(WITH_SOURCE_TREE "hierarchical")
macro(group_sources)
  # Skip this if WITH_SOURCE_TREE is not set (empty string).
  if (NOT ${WITH_SOURCE_TREE} STREQUAL "")
    foreach(dir ${ARGN})
      # Include all header and c files
      file(GLOB_RECURSE elements RELATIVE ${dir} ${dir}/*)

      foreach(element ${elements})
        # Extract filename and directory
        get_filename_component(element_name ${element} NAME)
        get_filename_component(element_dir ${element} DIRECTORY)

        if (NOT ${element_dir} STREQUAL "")
          # If the file is in a subdirectory use it as source group.
          if (${WITH_SOURCE_TREE} STREQUAL "flat")
            # Build flat structure by using only the first subdirectory.
            string(FIND ${element_dir} "/" delemiter_pos)
            if (NOT ${delemiter_pos} EQUAL -1)
              string(SUBSTRING ${element_dir} 0 ${delemiter_pos} group_name)
              source_group("${group_name}" FILES ${dir}/${element})
            else()
              # Build hierarchical structure.
              # File is in root directory.
              source_group("${element_dir}" FILES ${dir}/${element})
            endif()
          else()
            # Use the full hierarchical structure to build source_groups.
            string(REPLACE "/" "\\" group_name ${element_dir})
            source_group("${group_name}" FILES ${dir}/${element})
          endif()
        else()
          # If the file is in the root directory, place it in the root source_group.
          source_group("\\" FILES ${dir}/${element})
        endif()
      endforeach()
    endforeach()
  endif()
endmacro()
