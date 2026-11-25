#!/usr/bin/env python3
#
# Doxygen input filter for the example README.md files (see FILTER_PATTERNS in Doxyfile).
#
# The READMEs are written for GitHub. This filter turns them into Doxygen pages without changing the files:
#   - the first heading gets the page label "example_<Name>" (Name = example folder), used by @ref / @subpage
#   - links to other examples ("../Name", "../Name/README.md") become @ref example_Name
#   - the link to "Building the examples" of the main README becomes @ref examples_build
#   - other relative links (Extras/, tools/, ...) become links to the files on GitHub
#   - the source code of the main file (main/<Name>.cpp) is appended in a "Source code" section
#
# Doxygen runs it as: readme_filter.py <path of the README.md>  (output on stdout)

import os
import re
import sys

# GitHub location of the repository files (used for links to files that are not part of the documentation)
GITHUB_TREE = "https://github.com/fdivitto/FabGL/tree/fabgl2"

path      = os.path.abspath(sys.argv[1])
exampleDir = os.path.dirname(path)                                   # .../examples/<Category>/<Name>
repoRoot   = os.path.normpath(os.path.join(exampleDir, "..", "..", ".."))
examples   = os.path.join(repoRoot, "examples")
name       = os.path.basename(exampleDir)

text = open(path, encoding="utf-8").read()

# page label on the first heading
text = re.sub(r"\A# (.*)", lambda m: "# %s {#example_%s}" % (m.group(1).strip(), name), text, count=1)


def fixLink(m):
  label, target = m.group(1), m.group(2)
  if re.match(r"[a-z]+:", target) or target.startswith("#") or target.startswith("@"):
    return m.group(0)                                                # absolute URL, anchor or Doxygen ref
  dest, _, anchor = target.partition("#")
  full = os.path.normpath(os.path.join(exampleDir, dest.replace("%20", " ")))
  if full == os.path.join(repoRoot, "README.md") and anchor == "building-the-examples":
    return "[%s](@ref examples_build)" % label
  # another example (its folder or its README)
  rel = os.path.relpath(full, examples).split(os.sep)
  if not rel[0].startswith("..") and len(rel) >= 2 and (len(rel) == 2 or rel[2:] == ["README.md"]):
    if os.path.exists(os.path.join(examples, rel[0], rel[1], "README.md")):
      return "[%s](@ref example_%s)" % (label, rel[1])
  # any other file of the repository: link to GitHub
  url = GITHUB_TREE + "/" + os.path.relpath(full, repoRoot).replace(os.sep, "/").replace(" ", "%20")
  return "[%s](%s%s)" % (label, url, "#" + anchor if anchor else "")


# links (not images: "![...](...)" is preceded by "!")
text = re.sub(r"(?<!!)\[([^\]]*)\]\(([^)\s]+)\)", fixLink, text)
text = text.replace("(@ref examples_build) in the main README", "(@ref examples_build)")

# Double quotes in normal text: for Doxygen a '"' starts a quoted string that ends at the next '"', and an
# unpaired one (as in: 8" floppy) disables the Markdown processing of the rest of the page (raw HTML shown).
# Escape them outside code spans and code blocks.
def escapeQuotes(line):
  parts = line.split("`")
  for i in range(0, len(parts), 2):          # even parts are outside code spans
    parts[i] = parts[i].replace('"', '\\"')
  return "`".join(parts)

lines   = text.split("\n")
inBlock = False
for i, line in enumerate(lines):
  if line.lstrip().startswith("```"):
    inBlock = not inBlock
  elif not inBlock:
    lines[i] = escapeQuotes(line)
text = "\n".join(lines)


# source code of the main file at the end of the page (EXAMPLE_PATH = ../examples/), so the page about an
# example shows both its explanation and its code
category = os.path.basename(os.path.dirname(exampleDir))
mainSrc  = os.path.join(exampleDir, "main", name + ".cpp")
if os.path.exists(mainSrc):
  folderURL = GITHUB_TREE + "/examples/%s/%s" % (category, name)
  text = text.rstrip("\n") + "\n\n\n" + \
         "## Source code\n\n" + \
         "Main file of the example, `main/%s.cpp`. The complete project is in the " % name + \
         "[examples/%s/%s](%s) folder.\n\n" % (category, name, folderURL) + \
         "@include{lineno} %s/%s/main/%s.cpp\n" % (category, name, name)

sys.stdout.write(text)
