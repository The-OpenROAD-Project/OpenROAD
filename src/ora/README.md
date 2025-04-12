# ORAssistant

ORAssistant is a chat assistant designed to provide quick and easy access to information about OpenROAD tools, answer questions, and address common issues related to OpenROAD and its native flow, OpenROAD-flow-scripts. The system leverages retrieval techniques on OpenROAD documentation and other online data sources to deliver accurate and relevant responses.
Commands

### ora_init

The ora_init command is used to configure user consent and set up the host URL for ORAssistant. It supports both cloud-hosted and local versions of the tool.

Cloud-hosted version:
  To use the cloud-hosted version and provide consent, run: `ora_init cloud y`
  To revoke consent and stop using the cloud-hosted version, run: `ora_init cloud n`

Local version:
  To use a locally hosted version of ORAssistant, run: `ora_init local hostUrl`
  Replace hostUrl with the backend URL of your locally hosted ORAssistant. For more details on setting up a local backend, visit https://github.com/The-OpenROAD-Project/ORAssistant/

### askbot

The askbot command allows users to submit queries to ORAssistant. It supports an optional `-listSources` flag to display the sources of information used in the response. The query is sent as a POST request to the configured ORAssistant host URL.

Example usage: `askbot "How do I resolve timing violations in OpenROAD?" -listSources`

This command sends the query to ORAssistant and, if the -listSources flag is included, lists the sources used to generate the response.
