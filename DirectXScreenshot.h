#ifndef DIRECTX_SCREENSHOT_H
#define DIRECTX_SCREENSHOT_H

#include <windows.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <vector>
#include <iostream>

class DirectXScreenShot 
{
public:
    DirectXScreenShot() = default;
    DirectXScreenShot(const DirectXScreenShot& other) = delete;
    DirectXScreenShot(DirectXScreenShot&& other) = default;

    DirectXScreenShot& operator=(const DirectXScreenShot& other) = delete;
    DirectXScreenShot& operator=(DirectXScreenShot&& other) = default;

    ~DirectXScreenShot();

    bool start();
    bool screen(std::vector<char>& screenshot);
    void stop();

    unsigned int getWidth() const;
    unsigned int getHeight() const;

private:
    bool started = false;

    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;
    IDXGIOutputDuplication* dxgiOutputDuplication = nullptr;

    unsigned int width = 0;
    unsigned int height = 0;
};

#endif

