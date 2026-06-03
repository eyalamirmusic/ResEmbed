# DIRECTORY mode (build-time file list) with the default bucket count: the 5
# resources are spread across 8 C translation units (3 end up empty).
add_executable(BucketsTest Main.cpp)
res_embed_add(BucketsTest DIRECTORY Resources)

# Same resources, but capped to 2 buckets via TU_COUNT: every resource still
# resolves, now round-robined across exactly two C TUs.
add_executable(BucketsTuCountTest Main.cpp)
res_embed_add(BucketsTuCountTest DIRECTORY Resources TU_COUNT 2)
