#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <switch.h>

// #include <curl.h>

// void network_request(void) {
//     CURL *curl;
//     CURLcode res;

//     curl_global_init(CURL_GLOBAL_ALL);
//     curl = curl_easy_init();

//     if (curl) {
//         curl_easy_setopt(curl, CURLOPT_URL, "https://example.com");
//         curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

//         printf("Performing network request...\n");
        
//         res = curl_easy_perform(curl);
//         if (res != CURLE_OK) {
//             printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
//         } else {
//             printf("Network request successful!\n");
//         }

//         curl_easy_cleanup(curl);
//     }

//     curl_global_cleanup();
// }


// Usage: copy_directory("save:/", "save:/COPY");
void copy_directory(const char *src_dir, const char *dest_dir) {
    // Open the source directory
    DIR *dir = opendir(src_dir);
    if (dir == NULL) {
        printf("Failed to open source directory: %s\n", src_dir);
        return;
    }

    // Iterate over all entries in the directory
    struct dirent *ent;
    while ((ent = readdir(dir))) {
        // Skip the "." and ".." entries
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        // Construct the source path
        char src_path[256];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, ent->d_name);

        // Construct the destination path
        char dest_path[256];
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, ent->d_name);

        // Check if the entry is a directory
        struct stat st;
        if (stat(src_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            // The entry is a directory, so create the destination directory and copy it recursively
            mkdir(dest_path, 0700);
            copy_directory(src_path, dest_path);
        } else {
            // The entry is a file, so copy it
            FILE *src_file = fopen(src_path, "r");
            FILE *dest_file = fopen(dest_path, "w");
            char buffer[4096];
            size_t bytes;
            while ((bytes = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
                fwrite(buffer, 1, bytes, dest_file);
            }
            fclose(src_file);
            fclose(dest_file);
        }
    }

    // Close the source directory
    closedir(dir);
}

Result get_save(u64 *application_id, AccountUid *uid) {
    Result rc=0;
    FsSaveDataInfoReader reader;
    s64 total_entries=0;
    FsSaveDataInfo info;
    bool found=0;

    rc = fsOpenSaveDataInfoReader(&reader, FsSaveDataSpaceId_User);// See libnx fs.h.
    if (R_FAILED(rc)) {
        printf("fsOpenSaveDataInfoReader() failed: 0x%x\n", rc);
        return rc;
    }

    // Find the first savedata with FsSaveDataType_SaveData.
    while(1) {
        rc = fsSaveDataInfoReaderRead(&reader, &info, 1, &total_entries);//See libnx fs.h.
        if (R_FAILED(rc) || total_entries==0) break;
        if (R_SUCCEEDED(rc)) {
            printf("Data: 0x%x\n", rc);
        }

        if (info.save_data_type == FsSaveDataType_Account && info.application_id == *application_id) {
            *uid = info.uid;
            found = 1;
            break;
        }
    }

    fsSaveDataInfoReaderClose(&reader);

    if (R_SUCCEEDED(rc) && !found) {
        printf("Save found");
        return MAKERESULT(Module_Libnx, LibnxError_NotFound);
    }

    return rc;
}

int main(int argc, char **argv)
{
    Result rc=0;

    DIR* dir;
    struct dirent* ent;

    AccountUid uid={0};
    u64 application_id=0x0100F2C0115B6000; // ApplicationId of the save to mount

    get_save(&application_id, &uid);

    consoleInit(NULL);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    PadState pad;
    padInitializeDefault(&pad);

    // Reading rood sdmc:/ dir
    dir = opendir("");
    if(dir==NULL)
    {
        printf("Failed to open dir.\n");
    }
    else
    {
        printf("Dir-listing for '':\n");
        while ((ent = readdir(dir)))
        {
            printf("d_name: %s\n", ent->d_name);
        }
        closedir(dir);
        printf("Done.\n");
    }

    // Get the userID for save mounting. To mount common savedata, use an all-zero userID.

    // Try to find savedata to use with get_save() first, otherwise fallback to the above hard-coded TID + the userID from accountGetPreselectedUser().
    if (R_FAILED(get_save(&application_id, &uid))) {
        rc = accountInitialize(AccountServiceType_Application);
        if (R_FAILED(rc)) {
            printf("accountInitialize() failed: 0x%x\n", rc);
        }

        if (R_SUCCEEDED(rc)) {
            rc = accountGetPreselectedUser(&uid);
            accountExit();

            if (R_FAILED(rc)) {
                printf("accountGetPreselectedUser() failed: 0x%x\n", rc);
            }
        }
    }

    if (R_SUCCEEDED(rc)) {
        printf("Using application_id=0x%016lx uid: 0x%lx 0x%lx\n", application_id, uid.uid[1], uid.uid[0]);
    }

    if (R_SUCCEEDED(rc)) {
        rc = fsdevMountSaveData("save", application_id, uid);
        if (R_FAILED(rc)) {
            printf("fsdevMountSaveData() failed: 0x%x\n", rc);
        }
    }


    if (R_SUCCEEDED(rc)) {
        // copy_directory("save:/", "save:/COPY");
        dir = opendir("save:/");

        // ------------ uncomment to write to file ------------
        // FILE *fp; // actually works somehow
        // fp = fopen("save:/hello.txt", "w"); 
        // fprintf(fp, "Hello World!\n");
        // fclose(fp);  
        
        if(dir==NULL)
        {
            printf("Failed to open dir.\n");
        }
        else
        {
            // FILE *fp2; - uncomment to write to file
            // fp2 = fopen("save:/TEST/directory.txt", "a");
            printf("Dir-listing for 'save:/':\n");
            while ((ent = readdir(dir)))
            {
                printf("d_name: %s\n", ent->d_name);
                // fprintf(fp2, "d_name: %s\n", ent->d_name);
            }
            // fclose(fp2);
            closedir(dir);
            printf("Done.\n");
        }

        fsdevUnmountDevice("save");
    }

    // Main loop
    while(appletMainLoop())
    {
        padUpdate(&pad);
        
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}