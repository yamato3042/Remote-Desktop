#include "DirectXScreenshot.h"

DirectXScreenShot::~DirectXScreenShot()
{
    stop();
}

bool DirectXScreenShot::start() 
{
    if (started)
        return false;

    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &d3dDevice, nullptr, &d3dContext);

    if (FAILED(hr)) 
        return false;

    IDXGIDevice* dxgiDevice = nullptr;
    IDXGIAdapter* dxgiAdapter = nullptr;
    IDXGIOutput* dxgiOutput = nullptr;
    IDXGIOutput1* dxgiOutput1 = nullptr;

    DXGI_OUTPUT_DESC outputDesc;

    d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    dxgiDevice->GetAdapter(&dxgiAdapter);
    dxgiAdapter->EnumOutputs(0, &dxgiOutput);
    dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&dxgiOutput1);

    dxgiOutput->GetDesc(&outputDesc);

    width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
    height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;

    hr = dxgiOutput1->DuplicateOutput(d3dDevice, &dxgiOutputDuplication);

    dxgiDevice->Release();
    dxgiDevice = nullptr;

    dxgiAdapter->Release();
    dxgiAdapter = nullptr;

    dxgiOutput->Release();
    dxgiOutput = nullptr;

    dxgiOutput1->Release();
    dxgiOutput1 = nullptr;

    if (FAILED(hr))
    {
        d3dContext->Release();
        d3dContext = nullptr;

        d3dDevice->Release();
        d3dDevice = nullptr;

        return false;
    }

    started = true;

    return true;
}

bool DirectXScreenShot::screen(std::vector<char>& screenshot) 
{
    if (!started)
        return false;

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* desktopResource = nullptr;

    HRESULT hr = dxgiOutputDuplication->AcquireNextFrame(10, &frameInfo, &desktopResource);

    if (FAILED(hr)) 
        return false;

    ID3D11Texture2D* desktopTexture = nullptr;

    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&desktopTexture));

    desktopResource->Release();
    desktopResource = nullptr;

    if (FAILED(hr))
    {
        dxgiOutputDuplication->ReleaseFrame();

        return false;
    }
    
    D3D11_TEXTURE2D_DESC desc;
    desktopTexture->GetDesc(&desc);

    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    ID3D11Texture2D* stagingTexture = nullptr;

    hr = d3dDevice->CreateTexture2D(&desc, nullptr, &stagingTexture);

    if (FAILED(hr)) 
    {
        desktopTexture->Release();
        desktopTexture = nullptr;

        dxgiOutputDuplication->ReleaseFrame();

        return false;
    }

    d3dContext->CopyResource(stagingTexture, desktopTexture);

    desktopTexture->Release();
    desktopTexture = nullptr;

    D3D11_MAPPED_SUBRESOURCE mapped;

    hr = d3dContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mapped);

    if (FAILED(hr)) 
    {
        stagingTexture->Release();
        stagingTexture = nullptr;

        dxgiOutputDuplication->ReleaseFrame();

        return false;
    }

    screenshot.resize(width * height * 3);

    char* src = reinterpret_cast<char*>(mapped.pData);
    char* dst = screenshot.data();

    for (unsigned int y = 0; y < height; y++) 
    {
        char* row = src + y * mapped.RowPitch;

        for (unsigned int x = 0; x < width; x++)
        {
            dst[2] = row[2];  
            dst[1] = row[1];  
            dst[0] = row[0];  

            row += 4;     
            dst += 3;
        }
    }

    d3dContext->Unmap(stagingTexture, 0);

    stagingTexture->Release();
    stagingTexture = nullptr;

    dxgiOutputDuplication->ReleaseFrame();

    return true;
}

void DirectXScreenShot::stop() 
{
    if (!started)
        return;

    started = false;
    width = 0;
    height = 0;

    if (dxgiOutputDuplication)
    {
        dxgiOutputDuplication->Release();
        dxgiOutputDuplication = nullptr;
    }

    if (d3dContext) 
    {
        d3dContext->Release();
        d3dContext = nullptr;
    }

    if (d3dDevice) 
    {
        d3dDevice->Release();
        d3dDevice = nullptr;
    }
}

unsigned int DirectXScreenShot::getHeight() const
{
    return height;
}

unsigned int DirectXScreenShot::getWidth() const
{
    return width;
}