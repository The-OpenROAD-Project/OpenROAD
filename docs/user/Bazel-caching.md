# Bazel Developer Guide: Caching

This document provides instructions for developers on how to use Bazel's caching features to accelerate local builds.

## Caching Strategy Overview

Our Bazel setup uses a multi-layered caching strategy to provide optimal performance for different types of users:

1.  **Local Cache (Default):** All builds use a local on-disk cache by default. This provides a speed-up for incremental builds without requiring any special configuration or authentication.
2.  **Remote Cache (OpenROAD Developers & CI):** We maintain a remote cache on Google Cloud Storage. This allows team members and CI jobs to share build artifacts, dramatically reducing build times.

---

### 1. Local On-Disk Cache

By default, all builds use a local on-disk cache to speed up incremental builds. The default location is `~/.cache/bazel-disk-cache`.

You can override this location by setting the `OPENROAD_BAZEL_CACHE` environment variable:

```bash
export OPENROAD_BAZEL_CACHE=/path/to/your/cache
bazel build //...
```

No remote cache is accessed, and no authentication is required for the local cache. This provides a seamless experience for external contributors.
Secure VMs already have this set up, so no additional configuration is needed.

---

### 2. OpenROAD Developer Access (for `@openroad.tools` users)

Team members can enable **read-only** access to the remote cache by using the `openroad-dev` configuration. This will dramatically speed up local builds by downloading artifacts already built by CI.

#### On Secure Development VMs

If you are working on a secure development VM provided by OpenROAD, you do **not** need to authenticate. The VM's service account already has the necessary read access to the cache.

#### On a Local Environment

If you are working on your local machine, you must first authenticate with Google Cloud using your @openroad.tools account. All OpenROAD team members have read access to the remote cache.

```bash
gcloud auth application-default login
```

After authenticating (if necessary), run your build with the `--config=openroad-dev` flag:

```bash
bazel build --config=openroad-dev //...
```

This configuration is **read-only** to prevent local, unverified builds from populating the shared cache.

### Using a `user.bazelrc` file instead of `--config=openroad-dev`

You may prefer `user.bazelrc` file instead of using the `--config=openroad-dev` option.

    # user: username@openroad.tools
    build --remote_cache=https://storage.googleapis.com/openroad-bazel-cache
    build --credential_helper=*.googleapis.com=%workspace%/etc/cred_helper.py
    build --remote_cache_compression=true
    build --remote_upload_local_results=false

To test the setup:

    $ etc/cred_helper.py test
    Running: gcloud auth print-access-token username@openroad.tools
    {
    "kind": "storage#testIamPermissionsResponse",
    "permissions": [
        "storage.buckets.get",
        "storage.objects.create"
    ]
    }

---

### 3. CI Access (Jenkins Pipeline)

The Jenkins pipeline uses a unified caching strategy for all builds. This is primarily for informational purposes, as developers will not typically use the `ci` configuration locally.

*   All CI builds use the `--config=ci` profile.
*   Builds on the `master` branch have **read/write** access, populating the remote cache with the latest artifacts.
*   Builds on PR branches are set to **read-only**.

---

## `.bazelrc` Configurations Summary

The caching behavior is controlled by two new configurations in the `.bazelrc` file:

*   **`build:ci` (Write Access):**
    *   `--remote_upload_local_results=true`: **Enables writing** to the remote cache.
    *   Uses service account credentials provided by Jenkins.

*   **`build:openroad-dev` (Read-Only Access):**
    -   `--remote_upload_local_results=false`: **Disables writing**, making the cache read-only.
    -   `--google_default_credentials=true`: Uses the developer's local `gcloud` credentials.

Both configurations include performance optimizations to reduce network traffic and improve build speeds. The primary mechanism for this is the "Build without the Bytes" (BwoB) feature, which minimizes the download of build artifacts from the remote cache.

There are two main BwoB settings:

*   `--remote_download_toplevel`: This is the default setting in Bazel. It downloads only the outputs of the top-level targets you specify in your build command. This is ideal for interactive development, where you need to use the final build artifacts (e.g., run a binary or inspect a generated file).

*   `--remote_download_minimal`: This is a more aggressive setting that downloads only the artifacts essential for the build to continue. It is primarily intended for CI environments, where you are only concerned with the success or failure of a build, not the artifacts themselves.

Our configurations use `--remote_download_toplevel` for developer builds (`openroad-dev`) and `--remote_download_minimal` for CI builds (`ci`) to provide the best balance of performance and usability for each environment.
