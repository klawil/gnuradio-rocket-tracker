CREATE OR REPLACE VIEW public.devices_state AS SELECT
	devices.name as device_name,
	devices.device_type,
	current_state.device_serial,
	current_state.max_created_at,
	current_state.combined_state
FROM (
	SELECT
		device_serial,
		MAX(created_at) as max_created_at,
		jsonb_object_agg(packet_type, packet_json) as combined_state
	FROM (
	SELECT DISTINCT ON (packet_json->'Serial', packet_json->'Type')
		CAST(packet_json->'Serial' as integer) as device_serial,
		CAST(packet_json->'Type' as smallint) as packet_type,
		packet_json,
		created_at
	FROM public.packets
	ORDER BY packet_json->'Serial', packet_json->'Type', packet_json->'RTime' DESC, packet_json->'Time' DESC
	) agg
	GROUP BY agg.device_serial
) current_state
	LEFT JOIN public.devices ON current_state.device_serial = devices.serial;
ALTER VIEW public.devices_state OWNER TO william;
GRANT ALL ON public.devices_state TO rockettracker;
GRANT ALL ON public.devices_state TO william;
