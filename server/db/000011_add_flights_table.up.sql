CREATE TABLE public.device_flights (
  serial integer NOT NULL,
  flight integer NOT NULL,
  first_packet timestamp(6) without time zone,
  last_packet timestamp(6) without time zone,
  PRIMARY KEY (serial, flight)
);

CREATE FUNCTION public.sync_packets_to_flights()
  RETURNS TRIGGER AS
$$
BEGIN
  IF NEW.packet_json->>'Type' = '4' THEN
    INSERT INTO public.device_flights (serial, flight, first_packet, last_packet)
    VALUES (CAST(NEW.packet_json->'Serial' as integer), CAST(NEW.packet_json->'Flight' as integer), NEW.created_at, NEW.created_at)
    ON CONFLICT(serial, flight) DO UPDATE SET last_packet = NEW.created_at;
  END IF;
  RETURN NULL;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER packet_flight_state
AFTER INSERT ON public.packets
FOR EACH ROW EXECUTE PROCEDURE public.sync_packets_to_flights();
