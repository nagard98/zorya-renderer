A DX11 real-time renderer developed as a way to experiment with interesting rendering and engine features. Uses a **Render Graph** to ease the implemenentation of new features by automatically managing transient resources (currently only textures, not constant buffers). One of the main features is the presence of multiple **subsurface scattering** techniques. Planning to also integrate a DX12 backend.

![feature](https://github.com/user-attachments/assets/410aa56c-cf69-4c0f-bf79-3e3761b39054)

## Subsurface Scattering

Multiple subsurface scattering techniques are supported, specifically:
- Jimenez Gaussian [[Jim09]](https://doi.org/10.1145/1609967.1609970)
- Jimenez Separable [[Jim15]](https://doi.org/10.1111/cgf.12529)
- Golubev [[Gol18]](https://advances.realtimerendering.com/s2018/Efficient%20screen%20space%20subsurface%20scattering%20Siggraph%202018.pdf)
- Golubev Plus (an extension developed during my [thesis](https://github.com/user-attachments/files/17081122/Tesi_Magistrale_Vuletic_stampa_finale.pdf) that improves performance)

| ![gauss_med](https://github.com/user-attachments/assets/9917cd25-475f-4614-bfef-c0df65a2200b) | ![separable_med](https://github.com/user-attachments/assets/d5ab5edb-9362-4368-a36d-3168d10a980c) | ![golubev_med](https://github.com/user-attachments/assets/b64404df-ebd5-4d01-8ff4-119c8724662b) | ![golubev+_med](https://github.com/user-attachments/assets/8c01b15f-2801-4f94-8d01-6578d90ea62b) |
|:-:|:-:|:-:|:-:|
