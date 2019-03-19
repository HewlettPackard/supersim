load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive", "http_file")

release = "1.8.1"
http_archive(
  name = "googletest",
  urls = ["https://github.com/google/googletest/archive/release-" + release + ".tar.gz"],
  strip_prefix = "googletest-release-" + release,
)

http_file(
  name = "cpplint_build",
  urls = ["https://raw.githubusercontent.com/nicmcd/pkgbuild/master/cpplint.BUILD"],
)

release = "1.3.0"
http_archive(
  name = "cpplint",
  urls = ["https://github.com/cpplint/cpplint/archive/" + release + ".tar.gz"],
  strip_prefix = "cpplint-" + release,
  build_file = "@cpplint_build//file:downloaded",
)

http_file(
  name = "zlib_build",
  urls = ["https://raw.githubusercontent.com/nicmcd/pkgbuild/master/zlib.BUILD"],
)

version = "1.2.11"
http_archive(
  name = "zlib",
  urls = ["https://www.zlib.net/zlib-" + version + ".tar.gz"],
  strip_prefix = "zlib-" + version,
  build_file = "@zlib_build//file:downloaded",
)

http_file(
  name = "jsoncpp_build",
  urls = ["https://raw.githubusercontent.com/nicmcd/pkgbuild/master/jsoncpp.BUILD"],
)

version = "1.8.4"
http_archive(
  name = "jsoncpp",
  urls = ["https://github.com/open-source-parsers/jsoncpp/archive/" + version + ".tar.gz"],
  strip_prefix = "jsoncpp-" + version,
  build_file = "@jsoncpp_build//file:downloaded",
)

hash = "7257aa6"
http_archive(
  name = "libprim",
  urls = ["https://github.com/nicmcd/libprim/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libprim-" + hash,
)

hash = "09ed856"
http_archive(
  name = "libcolhash",
  urls = ["https://github.com/nicmcd/libcolhash/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libcolhash-" + hash,
)

hash = "b8a5894"
http_archive(
  name = "libfactory",
  urls = ["https://github.com/nicmcd/libfactory/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libfactory-" + hash,
)

hash = "cd2e7d9"
http_archive(
  name = "librnd",
  urls = ["https://github.com/nicmcd/librnd/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-librnd-" + hash,
)

hash = "d27b8c5"
http_archive(
  name = "libmut",
  urls = ["https://github.com/nicmcd/libmut/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libmut-" + hash,
)

hash = "b50b02d"
http_archive(
  name = "libbits",
  urls = ["https://github.com/nicmcd/libbits/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libbits-" + hash,
)

hash = "5cf3ada"
http_archive(
  name = "libstrop",
  urls = ["https://github.com/nicmcd/libstrop/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libstrop-" + hash,
)

hash = "f781c6b"
http_archive(
  name = "libfio",
  urls = ["https://github.com/nicmcd/libfio/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libfio-" + hash,
)

hash = "267a99d"
http_archive(
  name = "libsettings",
  urls = ["https://github.com/nicmcd/libsettings/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libsettings-" + hash,
)
