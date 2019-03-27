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

hash = "6b56ef3"
http_archive(
  name = "libprim",
  urls = ["https://github.com/nicmcd/libprim/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libprim-" + hash,
)

hash = "9d1d378"
http_archive(
  name = "libcolhash",
  urls = ["https://github.com/nicmcd/libcolhash/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libcolhash-" + hash,
)

hash = "8ae921c"
http_archive(
  name = "libfactory",
  urls = ["https://github.com/nicmcd/libfactory/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libfactory-" + hash,
)

hash = "55db323"
http_archive(
  name = "librnd",
  urls = ["https://github.com/nicmcd/librnd/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-librnd-" + hash,
)

hash = "8a7b8e7"
http_archive(
  name = "libmut",
  urls = ["https://github.com/nicmcd/libmut/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libmut-" + hash,
)

hash = "8127531"
http_archive(
  name = "libbits",
  urls = ["https://github.com/nicmcd/libbits/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libbits-" + hash,
)

hash = "ad29c47"
http_archive(
  name = "libstrop",
  urls = ["https://github.com/nicmcd/libstrop/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libstrop-" + hash,
)

hash = "528a0a3"
http_archive(
  name = "libfio",
  urls = ["https://github.com/nicmcd/libfio/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libfio-" + hash,
)

hash = "d7c6272"
http_archive(
  name = "libsettings",
  urls = ["https://github.com/nicmcd/libsettings/tarball/" + hash],
  type = "tar.gz",
  strip_prefix = "nicmcd-libsettings-" + hash,
)
