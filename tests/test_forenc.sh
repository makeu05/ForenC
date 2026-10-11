#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BIN="$ROOT/forenc"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

PASS=0
FAIL=0

pass() {
    printf '[PASS] %s\n' "$1"
    PASS=$((PASS + 1))
}

fail() {
    printf '[FAIL] %s\n' "$1"
    FAIL=$((FAIL + 1))
}

# 1. Vérifier la version
if "$BIN" --version | grep -q '0.2.0'; then
    pass "Version ForenC"
else
    fail "Version ForenC"
fi

# 2. Vérifier SHA-256
printf 'abc' > "$TMP/hash.txt"
EXPECTED='ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad'

if "$BIN" hash "$TMP/hash.txt" | grep -q "$EXPECTED"; then
    pass "Hachage SHA-256"
else
    fail "Hachage SHA-256"
fi

# Préparer les fichiers de référence
mkdir -p "$TMP/evidence"
printf 'stable\n' > "$TMP/evidence/stable.txt"
printf 'ancienne version\n' > "$TMP/evidence/modified.txt"
printf 'a supprimer\n' > "$TMP/evidence/deleted.txt"

"$BIN" scan "$TMP/evidence" -o "$TMP/baseline.json" >/dev/null

# Modifier le même répertoire pour conserver les chemins
printf 'nouvelle version\n' > "$TMP/evidence/modified.txt"
rm "$TMP/evidence/deleted.txt"
printf 'nouveau fichier\n' > "$TMP/evidence/added.txt"

"$BIN" scan "$TMP/evidence" -o "$TMP/followup.json" >/dev/null

# 3. Vérifier un manifeste valide
if "$BIN" verify "$TMP/followup.json" >/dev/null; then
    pass "Vérification d'intégrité sans modification"
else
    fail "Vérification d'intégrité sans modification"
fi

# 4. Vérifier la détection d'une modification
printf 'contenu altéré\n' > "$TMP/evidence/stable.txt"
if "$BIN" verify "$TMP/baseline.json" >"$TMP/verify.log" 2>&1; then
    fail "Détection d'un fichier modifié"
else
    if grep -q 'MODIFIED' "$TMP/verify.log"; then
        pass "Détection d'un fichier modifié"
    else
        fail "Détection d'un fichier modifié"
    fi
fi

# 5. Comparer les manifestes
if "$BIN" diff "$TMP/baseline.json" "$TMP/followup.json" \
    >"$TMP/diff.log"; then
    if grep -q 'ADDED.*added.txt' "$TMP/diff.log" &&
       grep -q 'DELETED.*deleted.txt' "$TMP/diff.log" &&
       grep -q 'MODIFIED.*modified.txt' "$TMP/diff.log" &&
       grep -q 'UNCHANGED.*stable.txt' "$TMP/diff.log"; then
        pass "Comparaison des quatre catégories"
    else
        fail "Comparaison des quatre catégories"
        cat "$TMP/diff.log"
    fi
else
    fail "Exécution de diff"
fi

# 6. Rejeter un manifeste invalide
printf '{invalid json' > "$TMP/invalid.json"
if "$BIN" diff "$TMP/invalid.json" "$TMP/followup.json" \
    >/dev/null 2>&1; then
    fail "Rejet d'un manifeste invalide"
else
    pass "Rejet d'un manifeste invalide"
fi

printf '\nRésultat : %s réussis, %s échoués\n' "$PASS" "$FAIL"

if [ "$FAIL" -ne 0 ]; then
    exit 1
fi
