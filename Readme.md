# What

After every call of

* xrEndFrame
* xrCreateSwapchain
* xrAcquireSwapchainImage
* xrWaitSwapchainImage
* xrReleaseSwapchainImage

this layer calls glXMakeCurrent with the `Display*`, `GLXContext` and `GLXDrawable` originally passed in the `XrGraphicsBindingOpenGLXlibKHR`.

Note that this might not be what the application actually expects.

# Why

https://github.com/ValveSoftware/SteamVR-for-Linux/issues/421

# Build

```
meson build
ninja -C build
```

# Installation

meson prints the one (two) commands required to use it, something like

```
mkdir -p ~/.local/share/openxr/1/api_layers/implicit.d/XrApiLayer_gl_context_fix.json
ln -s ~/gl_context_fix_layer/build/XrApiLayer_gl_context_fix.json ~/.local/share/openxr/1/api_layers/implicit.d/XrApiLayer_gl_context_fix.json
```

No installation necessary or supported. `XrApiLayer_gl_context_fix.json` points directly at the .so in the build directory.

# Status

Only `XrGraphicsBindingOpenGLXlibKHR` and only Linux supported.
