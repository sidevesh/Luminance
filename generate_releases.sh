#!/bin/sh

# Check if git is available and we are in a git repository
if command -v git >/dev/null 2>&1 && git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    # Get tags sorted by version (descending)
    git for-each-ref --sort=-version:refname --format '%(refname:short) %(creatordate:short)' refs/tags | while read -r version date; do
        # Clean up version string if it starts with 'v'
        clean_version="${version#v}"
        echo "    <release version=\"$clean_version\" date=\"$date\"/>"
    done
else
    # Fallback or empty if not a git repo
    echo ""
fi
