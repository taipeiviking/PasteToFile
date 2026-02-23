#include "ImageWritePng.h"

#include <windows.h>
#include <wincodec.h>

#include "PasteToFileCommon/Filename.h"
#include "PasteToFileCommon/Logging.h"

namespace ptf_helper {

static std::wstring NextCandidate(const std::wstring& dir,
                                  const std::wstring& baseName,
                                  int attempt) {
  if (attempt <= 0) return dir + L"\\" + baseName + L".png";
  wchar_t suffix[16]{};
  swprintf_s(suffix, L"-%02d", attempt);
  return dir + L"\\" + baseName + suffix + L".png";
}

static bool SaveHbitmapToPngFile(HBITMAP hbm, const std::wstring& path) {
  IWICImagingFactory* factory = nullptr;
  HRESULT hr =
      CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                       IID_PPV_ARGS(&factory));
  if (FAILED(hr) || !factory) return false;

  IWICBitmap* wicBitmap = nullptr;
  hr = factory->CreateBitmapFromHBITMAP(hbm, nullptr, WICBitmapUseAlpha,
                                        &wicBitmap);
  if (FAILED(hr) || !wicBitmap) {
    factory->Release();
    return false;
  }

  IWICStream* stream = nullptr;
  hr = factory->CreateStream(&stream);
  if (FAILED(hr) || !stream) {
    wicBitmap->Release();
    factory->Release();
    return false;
  }

  hr = stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE);
  if (FAILED(hr)) {
    stream->Release();
    wicBitmap->Release();
    factory->Release();
    return false;
  }

  IWICBitmapEncoder* encoder = nullptr;
  hr = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
  if (FAILED(hr) || !encoder) {
    stream->Release();
    wicBitmap->Release();
    factory->Release();
    return false;
  }

  hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
  if (FAILED(hr)) {
    encoder->Release();
    stream->Release();
    wicBitmap->Release();
    factory->Release();
    return false;
  }

  IWICBitmapFrameEncode* frame = nullptr;
  IPropertyBag2* props = nullptr;
  hr = encoder->CreateNewFrame(&frame, &props);
  if (props) props->Release();
  if (FAILED(hr) || !frame) {
    encoder->Release();
    stream->Release();
    wicBitmap->Release();
    factory->Release();
    return false;
  }

  hr = frame->Initialize(nullptr);
  if (SUCCEEDED(hr)) hr = frame->WriteSource(wicBitmap, nullptr);
  if (SUCCEEDED(hr)) hr = frame->Commit();
  if (SUCCEEDED(hr)) hr = encoder->Commit();

  frame->Release();
  encoder->Release();
  stream->Release();
  wicBitmap->Release();
  factory->Release();

  return SUCCEEDED(hr);
}

static bool SaveEncodedImageBytesToPngFile(const std::vector<uint8_t>& bytes,
                                           const std::wstring& path) {
  IWICImagingFactory* factory = nullptr;
  HRESULT hr =
      CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                       IID_PPV_ARGS(&factory));
  if (FAILED(hr) || !factory) return false;

  // Create an IStream over the in-memory bytes.
  HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE, bytes.size());
  if (!hglob) {
    factory->Release();
    return false;
  }
  void* mem = GlobalLock(hglob);
  if (!mem) {
    GlobalFree(hglob);
    factory->Release();
    return false;
  }
  memcpy(mem, bytes.data(), bytes.size());
  GlobalUnlock(hglob);

  IStream* inStream = nullptr;
  hr = CreateStreamOnHGlobal(hglob, TRUE /*fDeleteOnRelease*/, &inStream);
  if (FAILED(hr) || !inStream) {
    GlobalFree(hglob);
    factory->Release();
    return false;
  }

  IWICBitmapDecoder* decoder = nullptr;
  hr = factory->CreateDecoderFromStream(inStream, nullptr,
                                        WICDecodeMetadataCacheOnDemand, &decoder);
  inStream->Release();
  if (FAILED(hr) || !decoder) {
    factory->Release();
    return false;
  }

  IWICBitmapFrameDecode* frame = nullptr;
  hr = decoder->GetFrame(0, &frame);
  if (FAILED(hr) || !frame) {
    decoder->Release();
    factory->Release();
    return false;
  }

  IWICStream* outStream = nullptr;
  hr = factory->CreateStream(&outStream);
  if (FAILED(hr) || !outStream) {
    frame->Release();
    decoder->Release();
    factory->Release();
    return false;
  }

  hr = outStream->InitializeFromFilename(path.c_str(), GENERIC_WRITE);
  if (FAILED(hr)) {
    outStream->Release();
    frame->Release();
    decoder->Release();
    factory->Release();
    return false;
  }

  IWICBitmapEncoder* encoder = nullptr;
  hr = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
  if (FAILED(hr) || !encoder) {
    outStream->Release();
    frame->Release();
    decoder->Release();
    factory->Release();
    return false;
  }

  hr = encoder->Initialize(outStream, WICBitmapEncoderNoCache);
  if (FAILED(hr)) {
    encoder->Release();
    outStream->Release();
    frame->Release();
    decoder->Release();
    factory->Release();
    return false;
  }

  IWICBitmapFrameEncode* outFrame = nullptr;
  IPropertyBag2* props = nullptr;
  hr = encoder->CreateNewFrame(&outFrame, &props);
  if (props) props->Release();
  if (FAILED(hr) || !outFrame) {
    encoder->Release();
    outStream->Release();
    frame->Release();
    decoder->Release();
    factory->Release();
    return false;
  }

  hr = outFrame->Initialize(nullptr);
  if (SUCCEEDED(hr)) hr = outFrame->WriteSource(frame, nullptr);
  if (SUCCEEDED(hr)) hr = outFrame->Commit();
  if (SUCCEEDED(hr)) hr = encoder->Commit();

  outFrame->Release();
  encoder->Release();
  outStream->Release();
  frame->Release();
  decoder->Release();
  factory->Release();

  return SUCCEEDED(hr);
}

bool WritePngFileUniqueFromHbitmap(const std::wstring& targetDir, HBITMAP hbm,
                                   std::wstring* outPath) {
  if (outPath) *outPath = L"";
  for (int attempt = 0; attempt < 1000; attempt++) {
    std::wstring path = NextCandidate(targetDir, ptf::BuildDatedBaseName(), attempt);
    // Use CREATE_NEW semantics by opening a handle first.
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                           CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
      DWORD err = GetLastError();
      if (err == ERROR_FILE_EXISTS || err == ERROR_ALREADY_EXISTS) continue;
      ptf::LogLine(L"CreateFile for PNG failed: " + path + L" err=" +
                   std::to_wstring(err));
      return false;
    }
    CloseHandle(h);

    if (SaveHbitmapToPngFile(hbm, path)) {
      if (outPath) *outPath = path;
      ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                        L"[Helper] wrote png " + path);
      return true;
    }

    // If WIC write failed, remove the placeholder file.
    DeleteFileW(path.c_str());
    ptf::LogLine(L"WIC write PNG failed: " + path);
    return false;
  }
  return false;
}

bool WritePngFileUniqueFromEncodedImageBytesWithBase(
    const std::wstring& targetDir, const std::wstring& baseName,
    const std::vector<uint8_t>& bytes, std::wstring* outPath) {
  if (outPath) *outPath = L"";
  for (int attempt = 0; attempt < 1000; attempt++) {
    std::wstring path = NextCandidate(targetDir, baseName, attempt);
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                           CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
      DWORD err = GetLastError();
      if (err == ERROR_FILE_EXISTS || err == ERROR_ALREADY_EXISTS) continue;
      ptf::LogLine(L"CreateFile for PNG failed: " + path + L" err=" +
                   std::to_wstring(err));
      return false;
    }
    CloseHandle(h);

    if (SaveEncodedImageBytesToPngFile(bytes, path)) {
      if (outPath) *outPath = path;
      ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                        L"[Helper] wrote png " + path);
      return true;
    }

    DeleteFileW(path.c_str());
    ptf::LogLine(L"WIC decode/encode PNG failed: " + path);
    return false;
  }
  return false;
}

} // namespace ptf_helper
