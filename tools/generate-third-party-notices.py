"""Writes THIRD-PARTY-NOTICES.txt for a deployed calibly folder.

The notices name every third-party component in the program folder with its
license, copyright and the full license text:
  * the hand-written part, licenses/third-party-notices.in.txt (calibly's
    direct dependencies: Qt, FFmpeg, TagLib, the MinGW runtime, icons);
  * the components Qt has built into its libraries (zlib, PCRE2, PDFium, ...),
    found in Qt's software bill of materials (<Qt>/sbom/*.spdx.json) for
    exactly the Qt DLLs and plugins in the folder. Their license texts come
    from Qt's attribution pages (<Qt docs>/*-attribution-*.html) or from the
    SBOM itself.
Every DLL in the folder must be covered; otherwise the script stops, so a new
dependency cannot be shipped without its notice.

Run by tools/make-release.ps1 after "cmake --install":
  python tools/generate-third-party-notices.py <deploy folder> <Qt prefix> <Qt docs> <version>
"""

import fnmatch
import glob
import html
import json
import os
import re
import sys
import textwrap

PROJECT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# DLLs that are not part of Qt's SBOM and are covered by the hand-written part.
HAND_WRITTEN = {
    "avcodec-*.dll", "avformat-*.dll", "avutil-*.dll", "swresample-*.dll", "swscale-*.dll",  # FFmpeg
    "libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll",                          # MinGW runtime
}
# Packages of the Chromium part of Qt PDF that are build tools or Qt's own glue.
NOT_SHIPPED = {"GN", "Internal-Components", "QtPdf", "nasm"}

# Links Qt's SBOM does not state (Qt 6.12): the FFmpeg plugin has its
# implementation (and FFmpeg) linked in, Qt PDF the Chromium parts (PDFium).
EXTRA_LINKS = {
    "QFFmpegMediaPlugin": ["FFmpegMediaPluginImplPrivate"],
    "Pdf": ["QtWebEngine-Chromium-Pdf-QtPdf"],
}
# Packages whose contents (CONTAINS) are shipped libraries.
FOLLOW_CONTAINS = {"QtWebEngine-Chromium-Pdf-QtPdf"}
# Libraries Qt builds in without an attribution entry in the SBOM: the id of
# their page in Qt's documentation (*-attribution-<id>.html).
BUNDLED_PAGES = {
    "BundledHarfbuzz": "harfbuzz-ng",
    "BundledLibpng": "libpng",
    "BundledLibjpeg": "libjpeg",
    "BundledPcre2": "pcre2",
    "BundledFreetype": "freetype",
    "BundledZLIB": "zlib",
}


def load_sboms(qt_prefix):
    """All SBOM documents as one graph: packages, files and relationships,
    keyed by (document namespace, SPDX id)."""
    packages, files, edges, extracted = {}, {}, {}, {}
    for path in sorted(glob.glob(os.path.join(qt_prefix, "sbom", "*.spdx.json"))):
        with open(path, encoding="utf-8") as f:
            doc = json.load(f)
        ns = doc["documentNamespace"]
        refs = {r["externalDocumentId"]: r["spdxDocument"] for r in doc.get("externalDocumentRefs", [])}

        def key(ref):
            if ref.startswith("DocumentRef-"):
                doc_id, _, element = ref.partition(":")
                return (refs.get(doc_id, doc_id), element)
            return (ns, ref)

        for p in doc.get("packages", []):
            packages[(ns, p["SPDXID"])] = p
        for f in doc.get("files", []):
            files[(ns, f["SPDXID"])] = f["fileName"]
        for info in doc.get("hasExtractedLicensingInfos", []):
            extracted[info["licenseId"]] = info.get("extractedText", "")
        for r in doc.get("relationships", []):
            edges.setdefault(key(r["spdxElementId"]), []).append((r["relationshipType"], key(r["relatedSpdxElement"])))
    return packages, files, edges, extracted


def comment_field(package, field):
    match = re.search(r"^\s*" + field + r": (.*)$", package.get("comment", ""), re.M)
    return match.group(1).strip() if match else ""


def attribution_text(docs_dir, attribution_id):
    """License text of a Qt attribution page, "" if there is none."""
    # The page names use "-" where some ids have "_" (dr_libs -> dr-libs).
    pages = set()
    for name in {attribution_id, attribution_id.replace("_", "-")}:
        pages.update(glob.glob(os.path.join(docs_dir, "*", "*-attribution-%s.html" % name)))
    for page in sorted(pages):
        with open(page, encoding="utf-8") as f:
            text = f.read()
        blocks = re.findall(r"<pre[^>]*>(.*?)</pre>", text, re.S)
        if blocks:
            return "\n\n".join(html.unescape(re.sub(r"<[^>]+>", "", b)).strip("\n") for b in blocks)
    return ""


def attribution_page_info(docs_dir, attribution_id):
    """Name, version and the copyright / license lines of a Qt attribution page."""
    pages = glob.glob(os.path.join(docs_dir, "*", "*-attribution-%s.html" % attribution_id))
    if not pages:
        return attribution_id, "", ""
    with open(sorted(pages)[0], encoding="utf-8") as f:
        page = f.read()
    title = html.unescape(re.search(r"<title>(.*?)</title>", page, re.S).group(1)).split(" | ")[0].strip()
    name, _, version_info = title.partition(", version ")
    # The page: "upstream version: X", copyright lines (as text or as a
    # first <pre> block), the license name, the license text (last <pre>).
    start = page.find("upstream version")
    pres = [m.span() for m in re.finditer(r"<pre[^>]*>.*?</pre>", page, re.S)]
    if start < 0 or not pres:
        return name.strip(), version_info.strip(), ""
    begin = pres[0][1] if len(pres) > 1 else start
    header = html.unescape(re.sub(r"<[^>]+>", " ", page[begin:pres[-1][0]]))
    header = re.sub(r"\s+", " ", header).replace("upstream version:", "").strip()
    if version_info and header.startswith(version_info):
        header = header[len(version_info):].strip()
    return name.strip(), version_info.strip(), header.strip(" .,")


def plain_qdoc(text):
    """Qt's attribution texts use QDoc links: \\l {url}{text} -> text (url)."""
    return re.sub(r"\\l\s*\{([^}]*)\}\{([^}]*)\}", r"\2 (\1)", text)


def extra_text(attribution_id):
    """License texts Qt's documentation lacks, kept in licenses/qt-extra."""
    path = os.path.join(PROJECT, "licenses", "qt-extra", attribution_id + ".txt")
    if os.path.exists(path):
        with open(path, encoding="utf-8") as f:
            return f.read()
    return ""


def main():
    deploy, qt_prefix, docs_dir, version = sys.argv[1:5]
    packages, files, edges, extracted = load_sboms(qt_prefix)

    # Deployed DLLs -> the SBOM package that contains them.
    by_file = {}
    for (ns, file_id), name in files.items():
        by_file[name.lstrip("./").replace("\\", "/")] = (ns, file_id)
    containing = {}
    for source, targets in edges.items():
        for kind, target in targets:
            if kind == "CONTAINS" and target in files:
                containing[target] = source

    shipped, uncovered = [], []
    for root, _, names in os.walk(deploy):
        for name in names:
            if not name.lower().endswith(".dll"):
                continue
            rel = os.path.relpath(os.path.join(root, name), deploy).replace(os.sep, "/")
            sbom_name = rel if "/" in rel else "bin/" + rel
            if sbom_name in by_file and by_file[sbom_name] in containing:
                shipped.append(containing[by_file[sbom_name]])
            elif not any(fnmatch.fnmatch(name, pattern) for pattern in HAND_WRITTEN):
                uncovered.append(rel)
    if uncovered:
        sys.exit("No license information for: " + ", ".join(sorted(uncovered)))

    # Everything the shipped Qt parts depend on.
    by_name = {}
    for k, p in packages.items():
        by_name.setdefault(p["name"], []).append(k)
    seen, queue = set(), list(shipped)
    while queue:
        node = queue.pop()
        if node in seen:
            continue
        seen.add(node)
        name = packages[node]["name"]
        follow = ("DEPENDS_ON", "CONTAINS") if name in FOLLOW_CONTAINS else ("DEPENDS_ON",)
        queue.extend(target for kind, target in edges.get(node, []) if kind in follow and target in packages)
        for extra in EXTRA_LINKS.get(name, []):
            queue.extend(by_name.get(extra, []))

    modules = sorted({packages[k]["name"] for k in shipped})
    components, missing = {}, []
    for node in seen:
        p = packages[node]
        spdx_id = p["SPDXID"]
        if "-bundled-3rdparty-module-" in spdx_id:
            # A library Qt builds in. Some have attribution packages of their
            # own (reached through DEPENDS_ON), the others only a Qt
            # documentation page.
            if p["name"] in BUNDLED_PAGES:
                attribution_id = BUNDLED_PAGES[p["name"]]
                name, version_info, header = attribution_page_info(docs_dir, attribution_id)
                text = attribution_text(docs_dir, attribution_id)
                if not text:
                    missing.append("%s (%s)" % (p["name"], attribution_id))
                    continue
                components[attribution_id] = (name, version_info, header, "", "", text.strip())
            elif not any(packages[k]["name"].startswith(p["name"] + "_Attribution_") for k in seen):
                missing.append("%s (bundled library without attribution: add it to BUNDLED_PAGES)" % p["name"])
            continue
        if "3rdparty-" in spdx_id:
            attribution_id = comment_field(p, "Id")
            if not attribution_id:
                continue
            name = comment_field(p, "Name") or p["name"]
            license_name = comment_field(p, "License") or p.get("licenseConcluded", "")
            text = (attribution_text(docs_dir, attribution_id) or extracted.get(p.get("licenseConcluded", ""), "")
                    or extra_text(attribution_id))
            usage = plain_qdoc(comment_field(p, "Qt usage"))
        elif p["name"].startswith("QtWebEngine-Chromium-Pdf-"):
            short = p["name"][len("QtWebEngine-Chromium-Pdf-"):]
            if short in NOT_SHIPPED:
                continue
            attribution_id = "pdf-" + short
            name = short
            license_name = "see the license text below"
            text = extracted.get(p.get("licenseConcluded", ""), "")
            usage = "Used in Qt PDF (rendering PDF files)."
        else:
            continue
        copyright_text = p.get("copyrightText", "")
        if copyright_text in ("", "NOASSERTION"):
            copyright_text = ""
        if not text:
            missing.append("%s (%s, %s)" % (name, attribution_id, p.get("licenseConcluded", "")))
            continue
        components[attribution_id] = (name, p.get("versionInfo", ""), license_name, copyright_text, usage, text.strip())

    if missing:
        sys.exit("No license text for:\n  " + "\n  ".join(sorted(missing))
                 + "\nAdd them as licenses/qt-extra/<id>.txt.")

    parts = []
    for attribution_id in sorted(components, key=lambda k: components[k][0].lower()):
        name, ver, license_name, copyright_text, usage, text = components[attribution_id]
        lines = ["-" * 78, name + (", version " + ver if ver and ver != "NOASSERTION" else "")]
        if usage:
            lines += textwrap.wrap(usage, 78)
        lines += textwrap.wrap("License: " + license_name, 78)
        if copyright_text:
            lines.append(copyright_text)
        lines += ["", text, ""]
        parts.append("\n".join(lines))

    with open(os.path.join(PROJECT, "licenses", "third-party-notices.in.txt"), encoding="utf-8") as f:
        template = f.read()
    result = (template.replace("@CALIBLY_VERSION@", version)
                      .replace("@QT_MODULES@", "\n".join(textwrap.wrap(", ".join(modules), 78)))
                      .replace("@QT_COMPONENTS@", "\n".join(parts)))
    out = os.path.join(deploy, "THIRD-PARTY-NOTICES.txt")
    with open(out, "w", encoding="utf-8-sig", newline="\r\n") as f:
        f.write(result)
    print("Written %s: %d Qt parts, %d components built into Qt" % (out, len(modules), len(components)))


if __name__ == "__main__":
    main()
