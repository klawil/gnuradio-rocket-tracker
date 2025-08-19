import { readFileSync } from 'fs';

import ApiDocPage from './apiDocPage';

export const metadata = {
  title: 'API Documentation',
  description: 'The APIs that power the website',
};

export default function Page() {
  const spec: { [propName: string]: unknown } =
    JSON.parse(readFileSync(__dirname + '/../../../../oas.json', 'utf-8'));

  return <ApiDocPage
    spec={spec}
  />;
}
