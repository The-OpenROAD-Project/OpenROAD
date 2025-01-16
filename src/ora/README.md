# ORAssistant

The OpenROAD chat assistant aims to provide easy and quick access to information regarding tools, responses to questions and commonly occurring problems in OpenROAD and its native flow OpenROAD-flow-scripts. The current architecture uses certain retrieval techniques on OpenROAD documentation and other online data sources.

## Commands

### ora_init

The `ora_init` command sets up user consent for using the ORAssistant and configures the host URL if a local version is preferred.

#### Parameter

`consent`: Indicates whether the user agrees to use the web-hosted version. Accepts values like "y" (yes) or "n" (no).

`hostUrl` (optional): The URL of the local ORAssistant host if the web-hosted version is declined.

### askbot

The `askbot` commands accepts a user query, optionally lists sources (-listSources flag), and sends the query as a POST request to the ORAssistant's host URL.