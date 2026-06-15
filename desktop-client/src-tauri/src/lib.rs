use serde::Serialize;

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
struct ActiveWindowInfo {
    process: String,
    title: String,
}

#[cfg(target_os = "windows")]
#[tauri::command]
fn active_window() -> Result<Option<ActiveWindowInfo>, String> {
    use std::ffi::OsString;
    use std::os::windows::ffi::OsStringExt;
    use std::path::Path;
    use windows_sys::Win32::Foundation::{CloseHandle, HWND};
    use windows_sys::Win32::System::Threading::{
        OpenProcess, QueryFullProcessImageNameW, PROCESS_QUERY_LIMITED_INFORMATION,
    };
    use windows_sys::Win32::UI::WindowsAndMessaging::{
        GetForegroundWindow, GetWindowTextLengthW, GetWindowTextW, GetWindowThreadProcessId,
    };

    fn read_window_title(hwnd: HWND) -> String {
        let length = unsafe { GetWindowTextLengthW(hwnd) };
        if length <= 0 {
            return String::new();
        }

        let mut buffer = vec![0u16; length as usize + 1];
        let copied = unsafe { GetWindowTextW(hwnd, buffer.as_mut_ptr(), buffer.len() as i32) };
        OsString::from_wide(&buffer[..copied as usize])
            .to_string_lossy()
            .into_owned()
    }

    fn read_process_name(process_id: u32) -> Result<Option<String>, String> {
        let handle = unsafe { OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, process_id) };
        if handle.is_null() {
            return Ok(None);
        }

        let mut buffer = vec![0u16; 32768];
        let mut length = buffer.len() as u32;
        let ok = unsafe { QueryFullProcessImageNameW(handle, 0, buffer.as_mut_ptr(), &mut length) };
        unsafe {
            CloseHandle(handle);
        }

        if ok == 0 {
            return Ok(None);
        }

        let path = OsString::from_wide(&buffer[..length as usize])
            .to_string_lossy()
            .into_owned();
        let name = Path::new(&path)
            .file_stem()
            .map(|value| value.to_string_lossy().into_owned());

        Ok(name)
    }

    let hwnd = unsafe { GetForegroundWindow() };
    if hwnd.is_null() {
        return Ok(None);
    }

    let mut process_id = 0u32;
    unsafe {
        GetWindowThreadProcessId(hwnd, &mut process_id);
    }

    if process_id == 0 {
        return Ok(None);
    }

    let Some(process) = read_process_name(process_id)? else {
        return Ok(None);
    };

    Ok(Some(ActiveWindowInfo {
        process,
        title: read_window_title(hwnd),
    }))
}

#[cfg(not(target_os = "windows"))]
#[tauri::command]
fn active_window() -> Result<Option<ActiveWindowInfo>, String> {
    Ok(None)
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_shell::init())
        .invoke_handler(tauri::generate_handler![active_window])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
