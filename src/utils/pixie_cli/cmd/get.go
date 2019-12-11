package cmd

import (
	"fmt"
	"os"
	"strings"

	"github.com/gogo/protobuf/jsonpb"
	"github.com/olekukonko/tablewriter"
	uuid "github.com/satori/go.uuid"
	log "github.com/sirupsen/logrus"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"
	"pixielabs.ai/pixielabs/src/vizier/services/query_broker/querybrokerpb"
)

func init() {
	GetCmd.Flags().StringP("output", "o", "", "Output format: one of: json|proto")
}

// GetCmd is the "get" command.
var GetCmd = &cobra.Command{
	Use:   "get",
	Short: "Get information about cluster/edge modules",
	Run: func(cmd *cobra.Command, args []string) {
		// TODO(zasgar): Improvement needed after we spec out multiple vizier/agents support.
		// Placeholder function until we spec out vizier/agent listing.
		if len(args) != 1 || args[0] != "pem" {
			log.Fatalln("only a single argument pem is allowed")
		}

		cloudAddr := viper.GetString("cloud_addr")
		format, _ := cmd.Flags().GetString("output")
		format = strings.ToLower(format)
		v := mustConnectDefaultVizier(cloudAddr)

		res, err := v.GetAgentInfo()
		if err != nil {
			log.WithError(err).Fatal("Failed to get agent info")
		}

		err = nil
		switch format {
		case "json":
			err = formatAgentResultsAsJSON(res)
		case "pb":
			var b []byte
			b, err = res.Marshal()
			if err == nil {
				fmt.Printf("%s", string(b))
			}
		case "pbtxt":
			fmt.Print(res.String())
		default:
			formatAgentResultsAsTable(res)
		}

		if err != nil {
			log.WithError(err).Fatalln("Failed to print results")
		}
	},
}

func formatAgentResultsAsJSON(r *querybrokerpb.AgentInfoResponse) error {
	m := jsonpb.Marshaler{}
	return m.Marshal(os.Stdout, r)
}

func formatAgentResultsAsTable(r *querybrokerpb.AgentInfoResponse) {
	fmt.Printf("Number of agents: %d\n", len(r.Info))

	table := tablewriter.NewWriter(os.Stdout)
	table.SetHeader([]string{"Agent ID", "Hostname", "Last Heartbeat (ms)", "State"})
	for _, agentInfo := range r.Info {
		id := uuid.FromStringOrNil(string(agentInfo.Agent.Info.AgentID.Data))
		hbTime := agentInfo.Status.NSSinceLastHeartbeat
		table.Append([]string{id.String(),
			agentInfo.Agent.Info.HostInfo.Hostname,
			fmt.Sprintf("%d", int(hbTime/1000000)),
			agentInfo.Status.State.String(),
		})
	}
	table.Render()
}
