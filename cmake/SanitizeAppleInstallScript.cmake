if(NOT DEFINED INPUT)
    message(FATAL_ERROR "INPUT is required")
endif()

if(NOT EXISTS "${INPUT}")
    message(FATAL_ERROR "Install script not found: ${INPUT}")
endif()

file(READ "${INPUT}" _content)

string(REGEX REPLACE
       "\n[ \t]*execute_process\\(COMMAND /usr/bin/install_name_tool\n[ \t]*-delete_rpath \"[^\"]*\"\n[ \t]*\"\\$\\{file\\}\"\\)"
       ""
       _content
       "${_content}")

file(WRITE "${INPUT}" "${_content}")
