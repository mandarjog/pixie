package cmd

import (
	"os"
	"strconv"

	"github.com/gofrs/uuid"
	log "github.com/sirupsen/logrus"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"

	"github.com/gogo/protobuf/types"
	"pixielabs.ai/pixielabs/src/cloud/cloudapipb"
	cliLog "pixielabs.ai/pixielabs/src/pixie_cli/pkg/utils"
	"pixielabs.ai/pixielabs/src/pixie_cli/pkg/vizier"
	"pixielabs.ai/pixielabs/src/utils"
)

func init() {
	ConfigCmd.PersistentFlags().StringP("cluster_id", "c", "", "The ID of the cluster to get/update the config for")

	UpdateConfigCmd.Flags().StringP("passthrough", "t", "", "Whether pasthrough should be enabled")
	viper.BindPFlag("passthrough", UpdateConfigCmd.Flags().Lookup("passthrough"))
	UpdateConfigCmd.Flags().StringP("auto_update", "u", "", "Whether auto-updates should be enabled")
	viper.BindPFlag("auto_update", UpdateConfigCmd.Flags().Lookup("auto_update"))

	ConfigCmd.AddCommand(GetConfigCmd)
	ConfigCmd.AddCommand(UpdateConfigCmd)
}

// ConfigCmd is the "config" command for geting/updating the cluster config.
var ConfigCmd = &cobra.Command{
	Use:   "config",
	Short: "Get/update the current cluster config",
}

// GetConfigCmd is the "config get" command.
var GetConfigCmd = &cobra.Command{
	Use:   "get",
	Short: "Get the config for a cluster",
	Run: func(cmd *cobra.Command, args []string) {
		// Check cluster ID.
		clusterID, _ := cmd.Flags().GetString("cluster_id")
		if clusterID == "" {
			cliLog.Error("Need to specify cluster ID in flags: --cluster_id=<cluster-id>")
			return
		}
		clusterUUID, err := uuid.FromString(clusterID)
		if err != nil {
			cliLog.Errorf("Invalid cluster ID: %s\n", err.Error())
			return
		}

		cloudAddr := viper.GetString("cloud_addr")
		l, err := vizier.NewLister(cloudAddr)
		if err != nil {
			// Using log.Fatal rather than CLI log in order to track this unexpected error in Sentry.
			log.WithError(err).Fatal("Failed to create Vizier lister")
		}

		vzInfo, err := l.GetVizierInfo(clusterUUID)
		if err != nil {
			// Using log.Fatal rather than CLI log in order to track this unexpected error in Sentry.
			log.WithError(err).Fatal("Failed to get Vizier info")
		}

		if len(vzInfo) == 0 {
			cliLog.Errorf("Invalid cluster ID: %s", clusterID)
			os.Exit(1)
		}

		cliLog.Infof("%s: %t", "PassthroughEnabled", vzInfo[0].Config.PassthroughEnabled)
		cliLog.Infof("%s: %t", "AutoUpdateEnabled", vzInfo[0].Config.AutoUpdateEnabled)
		return
	},
}

// UpdateConfigCmd is the "config update" command.
var UpdateConfigCmd = &cobra.Command{
	Use:   "update",
	Short: "Update the config for a cluster",
	Run: func(cmd *cobra.Command, args []string) {
		// Check cluster ID.
		clusterID, _ := cmd.Flags().GetString("cluster_id")
		if clusterID == "" {
			cliLog.Error("Need to specify cluster ID in flags: --cluster_id=<cluster-id>")
			return
		}
		clusterUUID, err := uuid.FromString(clusterID)
		if err != nil {
			cliLog.Errorf("Invalid cluster ID: %s\n", err.Error())
			return
		}
		clusterIDPb := utils.ProtoFromUUID(clusterUUID)

		cloudAddr := viper.GetString("cloud_addr")
		l, err := vizier.NewLister(cloudAddr)
		if err != nil {
			// Using log.Fatal rather than CLI log in order to track this unexpected error in Sentry.
			log.WithError(err).Fatal("Failed to create Vizier lister")
		}

		ptEnabled, _ := cmd.Flags().GetString("passthrough")
		auEnabled, _ := cmd.Flags().GetString("auto_update")

		if ptEnabled == "" && auEnabled == "" {
			return // No config settings specified.
		}

		update := &cloudapipb.VizierConfigUpdate{}

		if ptEnabled != "" {
			if pt, err := strconv.ParseBool(ptEnabled); err == nil {
				update.PassthroughEnabled = &types.BoolValue{Value: pt}
			} else {
				cliLog.Errorf("Invalid value provided for passthrough: %s", err.Error())
			}
		}
		if auEnabled != "" {
			if au, err := strconv.ParseBool(auEnabled); err == nil {
				update.AutoUpdateEnabled = &types.BoolValue{Value: au}
			} else {
				cliLog.Errorf("Invalid value provided for auto_update: %s", err.Error())
			}
		}

		req := &cloudapipb.UpdateClusterVizierConfigRequest{
			ID:           clusterIDPb,
			ConfigUpdate: update,
		}

		err = l.UpdateVizierConfig(req)
		if err != nil {
			cliLog.Errorf("Error updating config: %s", err.Error())
		}
		cliLog.Info("Successfully updated config")
	},
}
