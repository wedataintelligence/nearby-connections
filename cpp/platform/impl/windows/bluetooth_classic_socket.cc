// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "platform/impl/windows/bluetooth_classic_socket.h"

#include "platform/impl/windows/generated/winrt/Windows.Networking.Sockets.h"

namespace location {
namespace nearby {
namespace windows {
BluetoothSocket::BluetoothSocket() {
  windows_socket_ = IStreamSocket();
  input_stream_ =
      std::make_unique<BluetoothInputStream>(windows_socket_.InputStream());
  output_stream_ =
      std::make_unique<BluetoothOutputStream>(windows_socket_.OutputStream());
}

BluetoothSocket::~BluetoothSocket() {}

// NOTE:
// It is an undefined behavior if GetInputStream() or GetOutputStream() is
// called for a not-connected BluetoothSocket, i.e. any object that is not
// returned by BluetoothClassicMedium::ConnectToService() for client side or
// BluetoothServerSocket::Accept() for server side of connection.
// Returns the InputStream of this connected BluetoothSocket.
InputStream& BluetoothSocket::GetInputStream() { return *input_stream_.get(); }

// Returns the OutputStream of this connected BluetoothSocket.
OutputStream& BluetoothSocket::GetOutputStream() {
  return *output_stream_.get();
}

// Closes both input and output streams, marks Socket as closed.
// After this call object should be treated as not connected.
// Returns Exception::kIo on error, Exception::kSuccess otherwise.
Exception BluetoothSocket::Close() {
  // The Close method aborts any pending operations and releases all unmanaged
  // resources associated with the StreamSocket object, including the Input and
  // Output streams
  windows_socket_.Close();
  windows_socket_ = nullptr;
  input_stream_ = nullptr;
  output_stream_ = nullptr;
  return {Exception::kSuccess};
}

// https://developer.android.com/reference/android/bluetooth/BluetoothSocket.html#getRemoteDevice()
// Returns valid BluetoothDevice pointer if there is a connection, and
// nullptr otherwise.
// TODO(b/184975123): replace with real implementation.
api::BluetoothDevice* BluetoothSocket::GetRemoteDevice() { return nullptr; }

// Starts an asynchronous operation on a StreamSocket object to connect to a
// remote network destination specified by a remote hostname and a remote
// service name.
winrt::Windows::Foundation::IAsyncAction BluetoothSocket::ConnectAsync(
    HostName connectionHostName, winrt::hstring connectionServiceName) {
  // https://docs.microsoft.com/en-us/uwp/api/windows.networking.sockets.streamsocket.connectasync?view=winrt-20348
  return windows_socket_.ConnectAsync(connectionHostName,
                                      connectionServiceName);
}

BluetoothSocket::BluetoothInputStream::BluetoothInputStream(
    IInputStream stream) {
  winrt_stream_ = stream;
}

ExceptionOr<ByteArray> BluetoothSocket::BluetoothInputStream::Read(
    std::int64_t size) {
  Buffer buffer = Buffer(size);

  winrt_stream_.ReadAsync(buffer, size, InputStreamOptions::None);

  DataReader dataReader = DataReader::FromBuffer(buffer);

  ByteArray data((char*)buffer.data(), buffer.Length());

  return ExceptionOr(data);
}

IAsyncAction BluetoothSocket::CancelIOAsync() {
  // Cancels pending reads and writes over a StreamSocket object.
  // https://docs.microsoft.com/en-us/uwp/api/windows.networking.sockets.streamsocket.cancelioasync?view=winrt-20348
  return windows_socket_.as<StreamSocket>().CancelIOAsync();
}

Exception BluetoothSocket::BluetoothInputStream::Close() {
  try {
    winrt_stream_.Close();
  } catch (std::exception exception) {
    return {Exception::kFailed};
  }

  return {Exception::kSuccess};
}

BluetoothSocket::BluetoothOutputStream::BluetoothOutputStream(
    IOutputStream stream) {
  winrt_stream_ = stream;
}

Exception BluetoothSocket::BluetoothOutputStream::Write(const ByteArray& data) {
  Buffer buffer = Buffer(data.size());

  std::memcpy(buffer.data(), data.data(), data.size());

  try {
    winrt_stream_.WriteAsync(buffer);
  } catch (std::exception exception) {
    return {Exception::kFailed};
  }

  return {Exception::kSuccess};
}

Exception BluetoothSocket::BluetoothOutputStream::Flush() {
  try {
    winrt_stream_.FlushAsync().get();
  } catch (std::exception exception) {
    return {Exception::kFailed};
  }

  return {Exception::kSuccess};
}

Exception BluetoothSocket::BluetoothOutputStream::Close() {
  try {
    winrt_stream_.Close();
  } catch (std::exception exception) {
    return {Exception::kFailed};
  }

  return {Exception::kSuccess};
}

}  // namespace windows
}  // namespace nearby
}  // namespace location
