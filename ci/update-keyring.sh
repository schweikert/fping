#!/bin/bash

set -e

# Configuration
GNU_KEYRING_URL="https://ftp.gnu.org/gnu/gnu-keyring.gpg"
TMP_KEYRING="gnu-keyring.gpg"
OUTPUT_KEYRING="ci/fping-deps.gpg"

# Maintainer emails to extract their keys from the GNU keyring.
MAINTAINER_EMAILS=(
    "zackw@panix.com"             # Autoconf: Zack Weinberg
    "karl@freefriends.org"        # Automake: Karl Berry
    "ileanadumi95@protonmail.com" # Libtool: Ileana Dumitrescu
)

# Step 1: Initialize an isolated environment to avoid side effects.
export GNUPGHOME="$(mktemp -d)"
chmod 700 "$GNUPGHOME"
echo "Initialized isolated GNUPGHOME at $GNUPGHOME"
cleanup() {
    rm -rf "$GNUPGHOME"
    rm -f "$TMP_KEYRING"
    echo "Cleaned up."
}
trap cleanup EXIT

# Step 2: Download the official GNU Keyring (relies on https certificate checking).
echo "Downloading GNU Keyring from $GNU_KEYRING_URL"...
wget -q -O "$TMP_KEYRING" "$GNU_KEYRING_URL"

# Step 3: Extract the specific keys we need.
echo "Extracting maintainer keys from GNU Keyring..."
for EMAIL in "${MAINTAINER_EMAILS[@]}"; do
    # Verify that the key exists in the keyring
    if ! gpg --no-default-keyring --keyring "./$TMP_KEYRING" --list-keys "$EMAIL" > /dev/null 2>&1; then
        echo "Error: No key found for $EMAIL in GNU Keyring!"
        exit 1
    fi
    echo "Found key(s) for $EMAIL"
done

# Export specific keys to our project keyring.
gpg --no-default-keyring --keyring "./$TMP_KEYRING" --export \
    "${MAINTAINER_EMAILS[@]}" \
    > "$OUTPUT_KEYRING"

echo "Success! Updated $OUTPUT_KEYRING with keys from the official GNU Keyring."
