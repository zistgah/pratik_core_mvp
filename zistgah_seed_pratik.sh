#!/usr/bin/env bash
###############################################################################
#  zistgah_seed_pratik.sh
#  Seed this repository as zistgah/pratik_core_mvp (GitHub Pages ->
#  https://zistgah.org/pratik_core_mvp/), mint a citable DOI + an
#  OpenTimestamps proof with Misty DOI, and write the DOI back into
#  README.md, CITATION.cff, codemeta.json and index.html.
#
#  SPDX-License-Identifier: GPL-3.0-or-later
#  © 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
#
#  USAGE   ./zistgah_seed_pratik.sh
#  FLAGS   DO_PUSH=1  push to the repo                     (default 1)
#          DO_MINT=0  mint a REAL PERMANENT Zenodo DOI     (default 0 = dry-run)
#          SANDBOX=0  when minting, use sandbox.zenodo.org (default 0)
#          ZENODO_TOKEN, ORCID  read only from env, never pasted, never stored
#  Nothing irreversible happens without an explicit typed confirmation.
###############################################################################
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"
DO_PUSH="${DO_PUSH:-1}"
DO_MINT="${DO_MINT:-0}"
SANDBOX="${SANDBOX:-0}"

GH_SLUG="zistgah/pratik_core_mvp"
VER="1.0.0"
ARTIFACT="$ROOT/pratik_core_mvp-$VER.zip"

say(){  printf '\n\033[1;33m=== %s ===\033[0m\n' "$*"; }
ok(){   printf '\033[1;36m  %s\033[0m\n' "$*"; }
warn(){ printf '\033[1;31m  !! %s\033[0m\n' "$*"; }
die(){  printf '\033[1;31mFATAL: %s\033[0m\n' "$*" >&2; exit 1; }
confirm(){ read -r -p "  $1 " a; [ "$a" = "$2" ]; }

say "0. PREFLIGHT"
command -v git >/dev/null 2>&1 || die "git not found"
command -v zip >/dev/null 2>&1 || die "zip not found"
[ -f "$ROOT/doi/misty.json" ] || die "missing doi/misty.json"
[ -f "$ROOT/src/main.cpp" ]   || die "run me from the repo root"
HAVE_MISTY=0; command -v misty >/dev/null 2>&1 && HAVE_MISTY=1 || warn "misty not installed (pipx install misty-doi) — DOI steps skipped"
HAVE_GH=0;    command -v gh    >/dev/null 2>&1 && HAVE_GH=1    || warn "gh not installed — push skipped"

# quick sanity: the CPU build must pass before we seed anything
if command -v g++ >/dev/null 2>&1; then
  say "0b. VERIFY BY EXECUTION (CPU build + harness)"
  g++ -std=c++20 -O2 -Iinclude src/cpu_backend.cpp src/main.cpp -o /tmp/pratik_seed_check -pthread
  /tmp/pratik_seed_check >/tmp/pratik_seed_check.log 2>&1 && tail -1 /tmp/pratik_seed_check.log \
    || die "harness failed — refusing to seed a broken build (see /tmp/pratik_seed_check.log)"
  rm -f /tmp/pratik_seed_check
fi

# ---------------------------------------------------------------------------
say "1. BUILD RELEASE ARTIFACT (zip + checksums)"
rm -f "$ROOT"/pratik_core_mvp-*.zip "$ROOT/SHA256SUMS.txt"
( cd "$ROOT" && find . -type f \
    -not -path './.git/*' -not -path './build/*' -not -path './.work*/*' \
    -not -name '*.zip' -not -name 'SHA256SUMS.txt' -not -path './provenance/*' \
    | sort | xargs sha256sum > SHA256SUMS.txt )
( cd "$ROOT" && zip -q -X -r "$ARTIFACT" . \
    -x '.git/*' 'build/*' '.work*/*' '*.zip' 'provenance/*' )
ok "artifact: $ARTIFACT ($(du -h "$ARTIFACT" | cut -f1))"

# ---------------------------------------------------------------------------
say "2. MISTY VALIDATE + DRY-RUN"
[ -n "${ORCID:-}" ] && python3 - "$ROOT/doi/misty.json" "$ORCID" <<'PY'
import json,sys
p,orcid=sys.argv[1],sys.argv[2]
d=json.load(open(p)); d["creators"][0]["orcid"]=orcid
json.dump(d,open(p,"w"),indent=2,ensure_ascii=False)
PY
DOI_ID=""; RECORD_URL=""
if [ "$HAVE_MISTY" -eq 1 ]; then
  misty validate -m "$ROOT/doi/misty.json" || die "misty.json invalid"
  misty publish -m "$ROOT/doi/misty.json" -f "$ARTIFACT" --dry-run \
        --package-dir "$ROOT/.doi-package" --output "$ROOT/result.dryrun.json" >/dev/null
  ok "dry-run package -> $ROOT/.doi-package"
fi

# ---------------------------------------------------------------------------
if [ "$DO_MINT" -eq 1 ] && [ "$HAVE_MISTY" -eq 1 ]; then
  say "3. MINT DOI  (IRREVERSIBLE)"
  [ -n "${ZENODO_TOKEN:-}" ] || die "DO_MINT=1 but ZENODO_TOKEN unset. export ZENODO_TOKEN=... (rehearse with SANDBOX=1)"
  EXTRA=(); [ "$SANDBOX" -eq 1 ] && { export ZENODO_SANDBOX=1; EXTRA+=(--sandbox); warn "SANDBOX: disposable test DOI"; } || warn "PRODUCTION: a Zenodo DOI is PERMANENT"
  if confirm "Type MINT to publish a $([ "$SANDBOX" -eq 1 ] && echo SANDBOX || echo PRODUCTION) DOI:" MINT; then
    misty publish -m "$ROOT/doi/misty.json" -f "$ARTIFACT" "${EXTRA[@]}" \
          --package-dir "$ROOT/.doi-package" --output "$ROOT/result.json"
    DOI_ID="$(python3 -c 'import json;print(json.load(open("'"$ROOT"'/result.json")).get("doi",""))' 2>/dev/null || true)"
    RECORD_URL="$(python3 -c 'import json;print(json.load(open("'"$ROOT"'/result.json")).get("record_url",""))' 2>/dev/null || true)"
    ok "DOI : ${DOI_ID:-?}"; ok "URL : ${RECORD_URL:-?}"
    say "3b. OPENTIMESTAMPS PROOF"
    mkdir -p "$ROOT/provenance"
    misty ots stamp "$ARTIFACT" || warn "ots stamp failed (needs misty-doi[ots] or the ots CLI)"
    [ -f "$ARTIFACT.ots" ] && mv "$ARTIFACT.ots" "$ROOT/provenance/" && ok "stamped -> provenance/ (run 'ots upgrade' later for the Bitcoin attestation)"
  else warn "mint aborted; nothing published"; fi
else
  say "3. MINT DOI — skipped (DO_MINT=1 to mint; SANDBOX=1 to rehearse)"
fi

# ---------------------------------------------------------------------------
say "4. WRITE DOI INTO REPO FILES"
if [ -n "$DOI_ID" ]; then DISP="$DOI_ID"; URL="${RECORD_URL:-https://doi.org/$DOI_ID}"
else DISP="pending"; URL="https://zistgah.org/pratik_core_mvp/"; fi
for f in README.md CITATION.cff codemeta.json index.html; do
  [ -f "$ROOT/$f" ] || continue
  python3 - "$ROOT/$f" "$DISP" "$URL" <<'PY'
import sys
p,doi,url=sys.argv[1],sys.argv[2],sys.argv[3]
s=open(p,encoding='utf-8').read()
s=s.replace("__PRATIK_DOI__",doi).replace("__PRATIK_RECORD_URL__",url)
open(p,"w",encoding='utf-8').write(s)
PY
done
ok "DOI written: $DISP"

# ---------------------------------------------------------------------------
if [ "$DO_PUSH" -eq 1 ] && [ "$HAVE_GH" -eq 1 ]; then
  say "5. SEED + PUSH -> $GH_SLUG"
  gh auth status >/dev/null 2>&1 || die "gh not authenticated. Run: gh auth login"
  [ -d "$ROOT/.git" ] || { git init -q "$ROOT"; git -C "$ROOT" branch -M main; }
  git -C "$ROOT" add -A
  git -C "$ROOT" -c user.name="Abhishek Choudhary" -c user.email="dev@ayeai.xyz" \
      commit -q -m "PRATIK Kernel Core MVP v$VER${DOI_ID:+ (DOI $DOI_ID)}" || warn "nothing to commit"
  git -C "$ROOT" branch -M main
  if gh repo view "$GH_SLUG" >/dev/null 2>&1; then
    git -C "$ROOT" remote add origin "https://github.com/$GH_SLUG.git" 2>/dev/null || true
    git -C "$ROOT" push -u origin main
  elif confirm "Repo $GH_SLUG absent. Type CREATE to create it public and push:" CREATE; then
    gh repo create "$GH_SLUG" --public --source="$ROOT" --remote=origin --push \
       -d "PRATIK Kernel Core — balanced-ternary, event-driven developmental kernel (PEDLER/AyeAI Triad)"
  else warn "push skipped; local repo ready at $ROOT"; fi
  gh api -X POST "repos/$GH_SLUG/pages" -f "source[branch]=main" -f "source[path]=/" >/dev/null 2>&1 \
    && ok "Pages enabled" || ok "Pages already enabled"
  ok "visualizer live shortly at https://zistgah.org/pratik_core_mvp/"
else
  say "5. PUSH — skipped (DO_PUSH=1 with gh authenticated)."
fi

# tidy the local checksum/zip working files (kept out of git by .gitignore anyway)
rm -f "$ROOT/SHA256SUMS.txt"
say "DONE"
[ -n "$DOI_ID" ] && ok "DOI: $DOI_ID  ($RECORD_URL)" || ok "DOI not minted this run (files show 'pending')."
ok "repo: https://github.com/$GH_SLUG   ·   viz: https://zistgah.org/pratik_core_mvp/"
