// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_hglobal.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/base/dragdrop/os_exchange_data_provider_win.h"
#include "url/gurl.h"

namespace ui {

// Test getting using the IDataObject COM API
TEST(OSExchangeDataWinTest, StringDataAccessViaCOM) {
  OSExchangeData data;
  std::wstring input = L"O hai googlz.";
  data.SetString(input);
  base::win::ScopedComPtr<IDataObject> com_data(
      OSExchangeDataProviderWin::GetIDataObject(data));

  FORMATETC format_etc =
      { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
  EXPECT_EQ(S_OK, com_data->QueryGetData(&format_etc));

  STGMEDIUM medium;
  EXPECT_EQ(S_OK, com_data->GetData(&format_etc, &medium));
  std::wstring output =
      base::win::ScopedHGlobal<wchar_t>(medium.hGlobal).get();
  EXPECT_EQ(input, output);
  ReleaseStgMedium(&medium);
}

// Test setting using the IDataObject COM API
TEST(OSExchangeDataWinTest, StringDataWritingViaCOM) {
  OSExchangeData data;
  std::wstring input = L"http://www.google.com/";

  base::win::ScopedComPtr<IDataObject> com_data(
      OSExchangeDataProviderWin::GetIDataObject(data));

  // Store data in the object using the COM SetData API.
  CLIPFORMAT cfstr_ineturl = RegisterClipboardFormat(CFSTR_INETURL);
  FORMATETC format_etc =
      { cfstr_ineturl, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
  STGMEDIUM medium;
  medium.tymed = TYMED_HGLOBAL;
  HGLOBAL glob = GlobalAlloc(GPTR, sizeof(wchar_t) * (input.size() + 1));
  size_t stringsz = input.size();
  SIZE_T sz = GlobalSize(glob);
  base::win::ScopedHGlobal<wchar_t> global_lock(glob);
  wchar_t* buffer_handle = global_lock.get();
  wcscpy_s(buffer_handle, input.size() + 1, input.c_str());
  medium.hGlobal = glob;
  medium.pUnkForRelease = NULL;
  EXPECT_EQ(S_OK, com_data->SetData(&format_etc, &medium, TRUE));

  // Construct a new object with the old object so that we can use our access
  // APIs.
  OSExchangeData data2(data.provider().Clone());
  EXPECT_TRUE(data2.HasURL(OSExchangeData::CONVERT_FILENAMES));
  GURL url_from_data;
  std::wstring title;
  EXPECT_TRUE(data2.GetURLAndTitle(
      OSExchangeData::CONVERT_FILENAMES, &url_from_data, &title));
  GURL reference_url(input);
  EXPECT_EQ(reference_url.spec(), url_from_data.spec());
}

// Verifies SetData invoked twice with the same data clobbers existing data.
TEST(OSExchangeDataWinTest, RemoveData) {
  OSExchangeData data;
  std::wstring input = L"http://www.google.com/";
  std::wstring input2 = L"http://www.google2.com/";

  base::win::ScopedComPtr<IDataObject> com_data(
      OSExchangeDataProviderWin::GetIDataObject(data));

  // Store data in the object using the COM SetData API.
  CLIPFORMAT cfstr_ineturl = RegisterClipboardFormat(CFSTR_INETURL);
  FORMATETC format_etc =
      { cfstr_ineturl, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
  STGMEDIUM medium;
  medium.tymed = TYMED_HGLOBAL;
  {
    HGLOBAL glob = GlobalAlloc(GPTR, sizeof(wchar_t) * (input.size() + 1));
    size_t stringsz = input.size();
    SIZE_T sz = GlobalSize(glob);
    base::win::ScopedHGlobal<wchar_t> global_lock(glob);
    wchar_t* buffer_handle = global_lock.get();
    wcscpy_s(buffer_handle, input.size() + 1, input.c_str());
    medium.hGlobal = glob;
    medium.pUnkForRelease = NULL;
    EXPECT_EQ(S_OK, com_data->SetData(&format_etc, &medium, TRUE));
  }
  // This should clobber the existing data.
  {
    HGLOBAL glob = GlobalAlloc(GPTR, sizeof(wchar_t) * (input2.size() + 1));
    size_t stringsz = input2.size();
    SIZE_T sz = GlobalSize(glob);
    base::win::ScopedHGlobal<wchar_t> global_lock(glob);
    wchar_t* buffer_handle = global_lock.get();
    wcscpy_s(buffer_handle, input2.size() + 1, input2.c_str());
    medium.hGlobal = glob;
    medium.pUnkForRelease = NULL;
    EXPECT_EQ(S_OK, com_data->SetData(&format_etc, &medium, TRUE));
  }
  EXPECT_EQ(1u, static_cast<DataObjectImpl*>(com_data.get())->size());

  // Construct a new object with the old object so that we can use our access
  // APIs.
  OSExchangeData data2(data.provider().Clone());
  EXPECT_TRUE(data2.HasURL(OSExchangeData::CONVERT_FILENAMES));
  GURL url_from_data;
  std::wstring title;
  EXPECT_TRUE(data2.GetURLAndTitle(
      OSExchangeData::CONVERT_FILENAMES, &url_from_data, &title));
  EXPECT_EQ(GURL(input2).spec(), url_from_data.spec());
}

TEST(OSExchangeDataWinTest, URLDataAccessViaCOM) {
  OSExchangeData data;
  GURL url("http://www.google.com/");
  data.SetURL(url, L"");
  base::win::ScopedComPtr<IDataObject> com_data(
      OSExchangeDataProviderWin::GetIDataObject(data));

  CLIPFORMAT cfstr_ineturl = RegisterClipboardFormat(CFSTR_INETURL);
  FORMATETC format_etc =
      { cfstr_ineturl, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
  EXPECT_EQ(S_OK, com_data->QueryGetData(&format_etc));

  STGMEDIUM medium;
  EXPECT_EQ(S_OK, com_data->GetData(&format_etc, &medium));
  std::wstring output =
      base::win::ScopedHGlobal<wchar_t>(medium.hGlobal).get();
  EXPECT_EQ(url.spec(), base::WideToUTF8(output));
  ReleaseStgMedium(&medium);
}

TEST(OSExchangeDataWinTest, MultipleFormatsViaCOM) {
  OSExchangeData data;
  std::string url_spec = "http://www.google.com/";
  GURL url(url_spec);
  std::wstring text = L"O hai googlz.";
  data.SetURL(url, L"Google");
  data.SetString(text);

  base::win::ScopedComPtr<IDataObject> com_data(
      OSExchangeDataProviderWin::GetIDataObject(data));

  CLIPFORMAT cfstr_ineturl = RegisterClipboardFormat(CFSTR_INETURL);
  FORMATETC url_format_etc =
      { cfstr_ineturl, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
  EXPECT_EQ(S_OK, com_data->QueryGetData(&url_format_etc));
  FORMATETC text_format_etc =
      { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
  EXPECT_EQ(S_OK, com_data->QueryGetData(&text_format_etc));

  STGMEDIUM medium;
  EXPECT_EQ(S_OK, com_data->GetData(&url_format_etc, &medium));
  std::wstring output_url =
      base::win::ScopedHGlobal<wchar_t>(medium.hGlobal).get();
  EXPECT_EQ(url.spec(), base::WideToUTF8(output_url));
  ReleaseStgMedium(&medium);

  // The text is supposed to be the raw text of the URL, _NOT_ the value of
  // |text|! This is because the URL is added first and thus takes precedence!
  EXPECT_EQ(S_OK, com_data->GetData(&text_format_etc, &medium));
  std::wstring output_text =
      base::win::ScopedHGlobal<wchar_t>(medium.hGlobal).get();
  EXPECT_EQ(url_spec, base::WideToUTF8(output_text));
  ReleaseStgMedium(&medium);
}

TEST(OSExchangeDataWinTest, EnumerationViaCOM) {
  OSExchangeData data;
  data.SetURL(GURL("http://www.google.com/"), L"");
  data.SetString(L"O hai googlz.");

  CLIPFORMAT cfstr_file_group_descriptor =
      RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
  CLIPFORMAT text_x_moz_url = RegisterClipboardFormat(L"text/x-moz-url");

  base::win::ScopedComPtr<IDataObject> com_data(
      OSExchangeDataProviderWin::GetIDataObject(data));
  base::win::ScopedComPtr<IEnumFORMATETC> enumerator;
  EXPECT_EQ(S_OK, com_data.get()->EnumFormatEtc(DATADIR_GET,
                                                enumerator.Receive()));

  // Test that we can get one item.
  {
    // Explictly don't reset the first time, to verify the creation state is
    // OK.
    ULONG retrieved = 0;
    FORMATETC elements_array[1];
    EXPECT_EQ(S_OK, enumerator->Next(1,
        reinterpret_cast<FORMATETC*>(&elements_array), &retrieved));
    EXPECT_EQ(1, retrieved);
    EXPECT_EQ(text_x_moz_url, elements_array[0].cfFormat);
  }

  // Test that we can get one item with a NULL retrieved value.
  {
    EXPECT_EQ(S_OK, enumerator->Reset());
    FORMATETC elements_array[1];
    EXPECT_EQ(S_OK, enumerator->Next(1,
        reinterpret_cast<FORMATETC*>(&elements_array), NULL));
    EXPECT_EQ(text_x_moz_url, elements_array[0].cfFormat);
  }

  // Test that we can get two items.
  {
    EXPECT_EQ(S_OK, enumerator->Reset());
    ULONG retrieved = 0;
    FORMATETC elements_array[2];
    EXPECT_EQ(S_OK, enumerator->Next(2,
        reinterpret_cast<FORMATETC*>(&elements_array), &retrieved));
    EXPECT_EQ(2, retrieved);
    EXPECT_EQ(text_x_moz_url, elements_array[0].cfFormat);
    EXPECT_EQ(cfstr_file_group_descriptor, elements_array[1].cfFormat);
  }

  // Test that we can skip the first item.
  {
    EXPECT_EQ(S_OK, enumerator->Reset());
    EXPECT_EQ(S_OK, enumerator->Skip(1));
    ULONG retrieved = 0;
    FORMATETC elements_array[1];
    EXPECT_EQ(S_OK, enumerator->Next(1,
        reinterpret_cast<FORMATETC*>(&elements_array), &retrieved));
    EXPECT_EQ(1, retrieved);
    EXPECT_EQ(cfstr_file_group_descriptor, elements_array[0].cfFormat);
  }

  // Test that we can skip the first item, and create a clone that matches in
  // this state, and modify the original without affecting the clone.
  {
    EXPECT_EQ(S_OK, enumerator->Reset());
    EXPECT_EQ(S_OK, enumerator->Skip(1));
    base::win::ScopedComPtr<IEnumFORMATETC> cloned_enumerator;
    EXPECT_EQ(S_OK, enumerator.get()->Clone(cloned_enumerator.Receive()));
    EXPECT_EQ(S_OK, enumerator.get()->Reset());

    {
      ULONG retrieved = 0;
      FORMATETC elements_array[1];
      EXPECT_EQ(S_OK, cloned_enumerator->Next(1,
          reinterpret_cast<FORMATETC*>(&elements_array), &retrieved));
      EXPECT_EQ(1, retrieved);
      EXPECT_EQ(cfstr_file_group_descriptor, elements_array[0].cfFormat);
    }

    {
      ULONG retrieved = 0;
      FORMATETC elements_array[1];
      EXPECT_EQ(S_OK, enumerator->Next(1,
          reinterpret_cast<FORMATETC*>(&elements_array), &retrieved));
      EXPECT_EQ(1, retrieved);
      EXPECT_EQ(text_x_moz_url, elements_array[0].cfFormat);
    }
  }
}

TEST(OSExchangeDataWinTest, TestURLExchangeFormatsViaCOM) {
  OSExchangeData data;
  std::string url_spec = "http://www.google.com/";
  GURL url(url_spec);
  std::wstring url_title = L"www.google.com";
  data.SetURL(url, url_title);

  // File contents access via COM
  base::win::ScopedComPtr<IDataObject> com_data(
      OSExchangeDataProviderWin::GetIDataObject(data));
  {
    CLIPFORMAT cfstr_file_contents =
        RegisterClipboardFormat(CFSTR_FILECONTENTS);
    FORMATETC format_etc =
        { cfstr_file_contents, NULL, DVASPECT_CONTENT, 0, TYMED_HGLOBAL };
    EXPECT_EQ(S_OK, com_data->QueryGetData(&format_etc));

    STGMEDIUM medium;
    EXPECT_EQ(S_OK, com_data->GetData(&format_etc, &medium));
    base::win::ScopedHGlobal<char> glob(medium.hGlobal);
    std::string output(glob.get(), glob.Size());
    std::string file_contents = "[InternetShortcut]\r\nURL=";
    file_contents += url_spec;
    file_contents += "\r\n";
    EXPECT_EQ(file_contents, output);
    ReleaseStgMedium(&medium);
  }
}

TEST(OSExchangeDataWinTest, FileContents) {
  OSExchangeData data;
  std::string file_contents("data\0with\0nulls", 15);
  data.SetFileContents(base::FilePath(L"filename.txt"), file_contents);

  OSExchangeData copy(data.provider().Clone());
  base::FilePath filename;
  std::string read_contents;
  EXPECT_TRUE(copy.GetFileContents(&filename, &read_contents));
  EXPECT_EQ(L"filename.txt", filename.value());
  EXPECT_EQ(file_contents, read_contents);
}

TEST(OSExchangeDataWinTest, CFHtml) {
  OSExchangeData data;
  GURL url("http://www.google.com/");
  std::wstring html(
      L"<HTML>\n<BODY>\n"
      L"<b>bold.</b> <i><b>This is bold italic.</b></i>\n"
      L"</BODY>\n</HTML>");
  data.SetHtml(html, url);

  // Check the CF_HTML too.
  std::string expected_cf_html(
      "Version:0.9\r\nStartHTML:0000000139\r\nEndHTML:0000000288\r\n"
      "StartFragment:0000000175\r\nEndFragment:0000000252\r\n"
      "SourceURL:http://www.google.com/\r\n<html>\r\n<body>\r\n"
      "<!--StartFragment-->");
  expected_cf_html += base::WideToUTF8(html);
  expected_cf_html.append("<!--EndFragment-->\r\n</body>\r\n</html>");

  FORMATETC format = Clipboard::GetHtmlFormatType().ToFormatEtc();
  STGMEDIUM medium;
  IDataObject* data_object = OSExchangeDataProviderWin::GetIDataObject(data);
  EXPECT_EQ(S_OK, data_object->GetData(&format, &medium));
  base::win::ScopedHGlobal<char> glob(medium.hGlobal);
  std::string output(glob.get(), glob.Size());
  EXPECT_EQ(expected_cf_html, output);
  ReleaseStgMedium(&medium);
}

TEST(OSExchangeDataWinTest, SetURLWithMaxPath) {
  OSExchangeData data;
  std::wstring long_title(L'a', MAX_PATH + 1);
  data.SetURL(GURL("http://google.com"), long_title);
}

TEST(OSExchangeDataWinTest, ProvideURLForPlainTextURL) {
  OSExchangeData data;
  data.SetString(L"http://google.com");

  OSExchangeData data2(data.provider().Clone());
  ASSERT_TRUE(data2.HasURL(OSExchangeData::CONVERT_FILENAMES));
  GURL read_url;
  std::wstring title;
  EXPECT_TRUE(data2.GetURLAndTitle(
      OSExchangeData::CONVERT_FILENAMES, &read_url, &title));
  EXPECT_EQ(GURL("http://google.com"), read_url);
}

}  // namespace ui
