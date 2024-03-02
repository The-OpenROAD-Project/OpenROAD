# Docs todo.

if(_get_compile_flags_description)
	return()
endif()
set(_get_compile_flags_description YES)

function(get_compile_flags )
  set(OPENROAD_GPU_COMPILED ${GPU} PARENT_SCOPE)
  set(OPENROAD_PYTHON_COMPILED ${BUILD_PYTHON} PARENT_SCOPE)
  set(OPENROAD_GUI_COMPILED ${BUILD_GUI} PARENT_SCOPE)
endfunction()