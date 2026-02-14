#!/bin/sh

# Output file path - relative to repository root
OUTPUT_FILE="releases.xml"

# Get the repository root directory (works even when script is called from subdirectories)
REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || echo ".")
OUTPUT_PATH="$REPO_ROOT/$OUTPUT_FILE"

# Check if releases.xml already exists
if [ -f "$OUTPUT_PATH" ]; then
    echo "$OUTPUT_FILE already exists. skipping generation."
    echo "Please edit it manually and add the latest version entry."
    exit 0
fi

# Check if git is available and we are in a git repository
if command -v git >/dev/null 2>&1 && git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    # Create temporary file
    TEMP_FILE=$(mktemp)
    
    # Get tags sorted by version (descending)
    git for-each-ref --sort=-version:refname --format '%(refname:short) %(creatordate:short)' refs/tags | while read -r version date; do
        # Clean up version string if it starts with 'v'
        clean_version="${version#v}"
        echo "    <release version=\"$clean_version\" date=\"$date\"/>"
    done > "$TEMP_FILE"
    
    # Write to output file
    mv "$TEMP_FILE" "$OUTPUT_PATH"
    echo "Generated $OUTPUT_FILE with $(wc -l < "$OUTPUT_PATH") releases"
else
    echo "Error: Not in a git repository or git is not available" >&2
    exit 1
fi
