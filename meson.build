project(
        'gl_context_fix_layer',
        ['cpp', 'c'],
        version: '0.1',
        license: 'BSL-1.0',
        meson_version: '>=0.49.0',
        default_options: [
                'cpp_std=c++17',
                'warning_level=3',
        ],
)

openxr_dep = dependency('openxr', required: true)

deps = [openxr_dep]

cc = meson.get_compiler('c')
if host_machine.system() == 'linux'
    dl_dep = cc.find_library('dl', required: false)
    deps += dl_dep

    glx_dep = dependency('glx')
    deps += glx_dep
else
endif

lib = library('XrApiLayer_gl_context_fix',
	'api_layer_gl_context_fix.cpp',
	dependencies: deps,
	)

# TODO: use install path
full_path = lib.full_path()

build_dir = meson.current_build_dir()

conf_data = configuration_data()
conf_data.set('library_path', full_path)
conf_file = configure_file(input : 'XrApiLayer_gl_context_fix.json.in',
	output : 'XrApiLayer_gl_context_fix.json',
	configuration : conf_data)

# TODO: cross platform instructions
message('===========================================================')
message('After building copy or symlink the layer manifest to a layer directory')
message('mkdir -p ~/.local/share/openxr/1/api_layers/implicit.d/XrApiLayer_gl_context_fix.json')
message('ln -s ' +  join_paths(build_dir, 'XrApiLayer_gl_context_fix.json') + ' ~/.local/share/openxr/1/api_layers/implicit.d/XrApiLayer_gl_context_fix.json')
message('Note: ~/.config/openxr/1/api_layers/implicit.d does not work')
message('===========================================================')
