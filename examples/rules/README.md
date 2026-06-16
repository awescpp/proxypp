# proxy++ Rule Examples

This directory contains example rule files for proxy++.

These files are provided for reference only. proxy++ does not automatically load rule files from this directory.

By default, `proxy++ <sub-command>` attempts to load `rules.json` from the installation root directory.

You can also specify a custom rule file explicitly, for example:

```bash
proxy++ http --rules-file <file_path>
````

When `--rules-file` is provided, proxy++ loads only the specified file and does not load the default `rules.json` from
the installation root directory.

## Schema

The rule file format is defined by the JSON Schema file:

```text
schemas/proxypp_rules_schema_v1.json
```

Please refer to the schema as the formal definition of the rule configuration format.
