#!/usr/bin/env bash

if [ $# -ne 1 ]; then
  echo "usage: $0 <VERSION>"
  exit 1
fi

cat > debian/changelog <<EOF
openroad ($1) UNRELEASED; urgency=low

$(git log -1 --pretty=format:"-- %an <%ae> on %ad")
EOF
