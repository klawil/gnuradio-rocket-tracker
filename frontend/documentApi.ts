import {
  readdirSync, writeFileSync
} from 'fs';
import { resolve } from 'path';

import TypescriptOAS, {
  OpenApiSpecData,
  createProgram
} from 'ts-oas';

const outputFileName = resolve('oas.json');
const basePath = resolve('types');

const baseOasSpec: OpenApiSpecData<'3.1.0'> = {
  tags: [
    {
      name: 'Devices',
      description: 'APIs for interacting with devices',
    },
  ],
};

// Find all of the definition files
const files = readdirSync(basePath);

// Build the functions to get typing
const tsProgram = createProgram(
  files.filter(f => f.endsWith('Api.ts')),
  {
    strictNullChecks: true,
  },
  basePath
);
const tsoas = new TypescriptOAS(
  tsProgram,
  {
    ignoreErrors: true,
    defaultNumberType: 'integer',
  }
);

// Build the OAS
const specObject = tsoas.getOpenApiSpec(
  [ /.Api$/, ],
  baseOasSpec
);
writeFileSync(outputFileName, JSON.stringify(specObject, null, 2));
