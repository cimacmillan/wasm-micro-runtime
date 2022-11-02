import * as vscode from 'vscode';
import * as os from 'os';
import * as path from 'path';
import * as fs from 'fs';
import { CheckIfDirectoryExist, DownloadFile, UnzipFile } from './directoryUtilities';

interface LLDBSupportEntry {
    destinationDir: string;
    downloadUrl: string;
}

const LLDB_RESOURCE_DIR = "resource/debug";
const LLDB_OS_SUPPORT: Partial<Record<NodeJS.Platform, LLDBSupportEntry>> = {
    "linux": {
        destinationDir: "linux",
        downloadUrl: "https://github.com/cimacmillan/wasm-micro-runtime/releases/download/WAMR-1.1.1/wamr-lldb-1.1.1-universal-macos-latest.zip"
    },
    "darwin": {
        destinationDir: "osx",
        downloadUrl: "https://github.com/cimacmillan/wasm-micro-runtime/releases/download/WAMR-1.1.1/wamr-lldb-1.1.1-universal-macos-latest.zip"
    }
};

const WamrLLDBNotSupportedError = () => new Error("WAMR LLDB is not supported on this platform");

export function isLLDBInstalled(context: vscode.ExtensionContext): boolean {
    const extensionPath = context.extensionPath;
    const lldbOSEntry = LLDB_OS_SUPPORT[os.platform()]
    if (!lldbOSEntry) {
        throw WamrLLDBNotSupportedError();
    }

    const lldbBinaryPath = path.join(extensionPath, LLDB_RESOURCE_DIR, lldbOSEntry.destinationDir, "bin/lldb");

    return CheckIfDirectoryExist(lldbBinaryPath);
}

function getLLDBUnzipFilePath(destinationFolder: string, filename: string) {
    const dirs = filename.split("/");
    if (dirs[0] == "inst") {
        dirs.shift();
    }

    return path.join(destinationFolder, dirs.join("/"));
}

export async function promptInstallLLDB(context: vscode.ExtensionContext) {
    const extensionPath = context.extensionPath;
    const setup_prompt = "setup";
    const skip_prompt = "skip";
    const response = await vscode.window.showWarningMessage('No LLDB instance found. Setup now?', setup_prompt, skip_prompt);

    if (response == skip_prompt) {
        return;
    }

    const lldbOSEntry = LLDB_OS_SUPPORT[os.platform()];

    if (!lldbOSEntry) {
        throw WamrLLDBNotSupportedError();
    }

    const { downloadUrl, destinationDir } = lldbOSEntry;
    const lldbDestinationFolder = path.join(extensionPath, LLDB_RESOURCE_DIR, destinationDir);
    const lldbZipPath = path.join(lldbDestinationFolder, "bundle.zip");

    vscode.window.showInformationMessage(`Downloading LLDB...`);

    await DownloadFile(downloadUrl, lldbZipPath);

    vscode.window.showInformationMessage(`LLDB downloaded to ${lldbZipPath}. Installing...`);

    const lldbFiles = await UnzipFile(lldbZipPath, filename => getLLDBUnzipFilePath(lldbDestinationFolder, filename));
    lldbFiles.forEach(file => fs.chmodSync(file, "0775"));

    vscode.window.showInformationMessage(`LLDB installed at ${lldbDestinationFolder}`);

    // Remove the bundle.zip
    fs.unlink(lldbZipPath, () => {});
}


