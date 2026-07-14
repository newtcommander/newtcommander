// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
	Windows Portable Devices Plugin for Open Salamander
	
	Copyright (c) 2015 Milan Kase <manison@manison.cz>
	Copyright (c) 2015 Open Salamander Authors
	
	wpdhelpers.h
	Helper functions.
*/

#pragma once

static inline HRESULT WINAPI WpdGetStringValue(
    _In_ IPortableDeviceValues* values,
    _In_ REFPROPERTYKEY key,
    _Out_ CFxString& s)
{
    PWSTR pwz;
    HRESULT hr;

    _ASSERTE(values != nullptr);

    hr = values->GetStringValue(key, &pwz);
    if (SUCCEEDED(hr))
    {
        // WPD names are Unicode; the narrow CFxString carries them as UTF-8 so
        // they reach Salamander as UTF-8 (plugin interface 104). A plain
        // "s = pwz" would convert through the ANSI code page and lose characters.
        int cch = ::WideCharToMultiByte(CP_UTF8, 0, pwz, -1, nullptr, 0, nullptr, nullptr);
        if (cch > 0)
        {
            ::WideCharToMultiByte(CP_UTF8, 0, pwz, -1, s.GetBuffer(cch), cch, nullptr, nullptr);
            s.ReleaseBuffer();
        }
        else
        {
            s.Empty();
        }
        ::CoTaskMemFree(pwz);
    }

    return hr;
}

static inline HRESULT WINAPI WpdGetDateTimeValue(
    _In_ IPortableDeviceValues* values,
    _In_ REFPROPERTYKEY key,
    _Out_ SYSTEMTIME& time)
{
    HRESULT hr;
    PROPVARIANT v;

    hr = values->GetValue(key, &v);
    if (SUCCEEDED(hr))
    {
        if (v.vt == VT_DATE)
        {
            if (!::VariantTimeToSystemTime(v.date, &time))
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }
        }
        else if (v.vt == VT_ERROR && FAILED(v.scode))
        {
            hr = v.scode;
        }
        else
        {
            _ASSERTE(0);
            hr = E_FAIL;
        }

        ::PropVariantClear(&v);
    }

    return hr;
}

static inline HRESULT WINAPI WpdGetDateTimeValue(
    _In_ IPortableDeviceValues* values,
    _In_ REFPROPERTYKEY key,
    _Out_ FILETIME& time)
{
    HRESULT hr;
    SYSTEMTIME st;

    hr = WpdGetDateTimeValue(values, key, st);
    if (SUCCEEDED(hr))
    {
        FILETIME local;

        if (!::SystemTimeToFileTime(&st, &local) || !::LocalFileTimeToFileTime(&local, &time))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}

static inline void WINAPI WpdAddKeys(
    IPortableDeviceKeyCollection* collection,
    const PROPERTYKEY* const* const keys,
    unsigned count)
{
    _ASSERTE(collection != nullptr);

    for (unsigned i = 0; i < count; i++)
    {
        HRESULT hr = collection->Add(*keys[i]);
        _ASSERTE(SUCCEEDED(hr));
    }
}

static inline IPortableDeviceKeyCollection* WINAPI WpdInitKeys(
    const PROPERTYKEY* const* const keys,
    unsigned count)
{
    HRESULT hr;
    ATL::CComPtr<IPortableDeviceKeyCollection> coll;

    hr = coll.CoCreateInstance(CLSID_PortableDeviceKeyCollection);
    _ASSERTE(SUCCEEDED(hr));

    WpdAddKeys(coll, keys, count);

    return coll.Detach();
}
