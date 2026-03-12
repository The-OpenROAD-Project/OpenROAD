# Bazel Build Notes

## ccache

Bazel's action cache is keyed on the full action graph, not file contents.
Changing a BUILD file, a flag, or running `bazel clean` invalidates cached
actions even when the C++ source is identical. Buck2 avoids this with
content-based hashing of inputs. Until Bazel gains a similar feature,
`--config=ccache` is the "content hashes at home" workaround: ccache
hashes the preprocessed source, so unchanged translation units get
near-instant cache hits regardless of action graph churn.

Install ccache, then:

```bash
bazel build --config=ccache //...
```

Or add to `user.bazelrc`:

```
build --config=ccache
```
