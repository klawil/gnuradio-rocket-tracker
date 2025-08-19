#!/bin/bash

for f in $(ls -A ../public); do
  if [[ "$f" != ".gitkeep" && "$f" != "tiles" ]]; then
    echo "$f"
    rm -rf "../public/$f"
  fi
done

npm run build

cp -r out/* ../public
