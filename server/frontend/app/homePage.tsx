'use client';

import L from 'leaflet';
import { ListDevicesApi, ParsedDeviceState, parseDeviceState } from "@/types/DevicesApi";
import { typeFetch } from "@/utils/typeFetch";
import { useCallback, useEffect, useState } from "react";
import { Col, Container, Form, Row, Spinner, Table } from "react-bootstrap";
import styles from './page.module.css';
import 'leaflet/dist/leaflet.css';

import { CircleMarker, MapContainer, Marker, Popup, TileLayer, useMap } from "react-leaflet";
import { FlightState } from "@/types/Altus";

// Fix for default icon issues
import icon from "leaflet/dist/images/marker-icon.png";
import iconShadow from "leaflet/dist/images/marker-shadow.png";
const DefaultIcon = L.icon({
  iconUrl: icon as unknown as string,
  shadowUrl: iconShadow as unknown as string,
});
L.Marker.prototype.options.icon = DefaultIcon;

const fadeTime = 15 * 60 * 1000;
const flightStateIcons: {
  [key in FlightState]: string;
} = {
  [0]: 'rocket.png',
  [1]: 'rocket.png',
  [2]: 'rocket-pad.png',
  [3]: 'rocket-boost.png',
  [4]: 'rocket-fast.png',
  [5]: 'rocket-coast.png',
  [6]: 'rocket-drogue.png',
  [7]: 'rocket-main.png',
  [8]: 'rocket-landed.png',
  [9]: 'rocket.png',
  [10]: 'rocket.png',
};

const initMapCenter: [ number, number ] = [
  40.8444,
  -119.1123,
];

function useUserLocation(): [ number, number ] | null {
  const [ loc, setLoc ] = useState<[number, number] | null>(null);

  useEffect(() => {
    navigator.geolocation.watchPosition(
      pos => setLoc([
        pos.coords.latitude,
        pos.coords.longitude,
      ]),
      err => console.error(`Error getting location`, err),
      {
        enableHighAccuracy: true,
      },
    );
  }, []);

  return loc;
}

function MapMarker({
  device,
}: Readonly<{
  device: ParsedDeviceState;
}>) {
  if (!device.CombinedState[5]?.Locked) {
    console.log('klawil', 'Did not render device', device);
    return <></>;
  }

  const opacity = Date.now() - device.MaxCreatedAt.getTime() >= fadeTime
    ? 0.5
    : 1;
  const iconPath = flightStateIcons[device.DeviceStateNum];

  return <Marker
    opacity={opacity}
    position={[
      device.CombinedState[5].Latitude,
      device.CombinedState[5].Longitude,
    ]}
    icon={L.icon({
      iconUrl: `/icons/${iconPath}`,
      iconSize: [ 20, 20 ],
      iconAnchor: [ 10, 10 ],
      popupAnchor: [ 0, -5 ],
    })}
  >
    <Popup>Test</Popup>
  </Marker>;
}

function CenterDevice({
  center,
  setCenter,
}: Readonly<{
  center: ParsedDeviceState | null;
  setCenter: () => void,
}>) {
  const map = useMap();

  useEffect(() => {
    if (center !== null) {
      if (center.CombinedState[5]?.Locked) {
        map.setView([
          center.CombinedState[5].Latitude,
          center.CombinedState[5].Longitude,
        ])
      }

      setCenter();
    }
  }, [ map, center, setCenter ]);

  return <></>;
}

export default function HomePage() {
  const [ isLoading, setIsLoading ] = useState<boolean>(false);
  const [ devices, setDevices ] = useState<ParsedDeviceState[] | null>(null);
  const [ deviceErr, setDeviceErr ] = useState<string | null>(null);
  const [ centerDev, setCenterDev ] = useState<ParsedDeviceState | null>(null);
  const [ loadAll, setLoadAll ] = useState<boolean>(false);

  const loadDevices = useCallback(async (loadAll: boolean = false) => {
    setIsLoading(true);
    setDeviceErr(null);
    try {
      const [ code, response ] = await typeFetch<ListDevicesApi>({
        path: '/api/devices/',
        method: 'GET',
        query: {
          all: `${loadAll}`,
        },
      });
      if (
        code !== 200 ||
        !response ||
        response.Msg !== '' ||
        !('Devices' in response)
      ) {
        console.log(response);
        throw new Error(`Code: ${code}, Response: ${response}`);
      }

      if (response.Devices === null) {
        setDevices([]);
      } else {
        setDevices(response.Devices.map(d => parseDeviceState(d)));
      }
    } catch (e) {
      setDeviceErr(`${e}`);
      console.error('Error loading devices', e);
    }
    setIsLoading(false);
  }, []);

  useEffect(() => {
    if (
      !isLoading &&
      devices === null &&
      deviceErr === null
    ) {
      loadDevices(loadAll);
    }
  }, [ isLoading, deviceErr, loadAll, devices, loadDevices ]);

  const showErr = deviceErr !== null ||
    (
      !isLoading &&
      (
        devices === null ||
        devices.length === 0
      )
    );
  const errMsg = deviceErr !== null
    ? deviceErr
    : 'No devices found';

  // const hasDevices = !isLoading && devices !== null && devices.length > 0;
  const hasGpsDevices = !isLoading && devices !== null
    && devices.find(dev => dev.CombinedState[5]?.Locked);

  const userLoc = useUserLocation();
  const resetCenter = () => {
    setCenterDev(null);
  }

  return (
    <Container fluid={true}>
      <h1 className="text-center">Altus Rocket Tracker</h1>

      {(hasGpsDevices || userLoc !== null) && <Row className='justify-content-center mb-3'><Col md={6}>
      {/* {hasGpsDevices && <Row className='justify-content-center mb-3'><Col md={6}> */}
        <MapContainer
          className={styles.map}
          center={userLoc !== null
            ? userLoc
            : hasGpsDevices
              ? [
                hasGpsDevices.CombinedState[5]?.Latitude || initMapCenter[0],
                hasGpsDevices.CombinedState[5]?.Longitude || initMapCenter[1],
              ]
              : initMapCenter}
          zoom={14}
          maxZoom={20}
          // minZoom={14}
        >
          <TileLayer
            maxZoom={18}
            // minZoom={14}
            attribution='Imagery &copy; Esri'
            url='/tiles/{z}-{y}-{x}.png'
          />
          <TileLayer
            maxZoom={20}
            // minZoom={14}
		        attribution='Streets &copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>'
            url='/osm/tile/{z}/{x}/{y}.png'
          />
          {devices !== null && devices
            .filter(device => device.CombinedState[5]?.Locked)
            .map(device => <MapMarker device={device} key={device.DeviceSerial} />)
          }
          {userLoc !== null && <CircleMarker
            center={userLoc}
            radius={5}
            fillOpacity={1}
            opacity={0}
            fillColor='red'
            color='red'
          />}
          <CenterDevice
            center={centerDev}
            setCenter={resetCenter}
          />
        </MapContainer>
      </Col></Row>}

      <Row><Col className='justify-content-center'>
        <Form.Check
          type="checkbox"
          checked={loadAll}
          onChange={e => {
            setLoadAll(e.target.checked);
            loadDevices(e.target.checked);
          }}
          label="Load all devices"
        />
      </Col></Row>

      <Table striped={true} className="text-center">
        <thead>
          <tr>
            <th>Serial</th>
            <th>Name</th>
            <th>Type</th>
            <th>State</th>
            <th>Last Seen</th>
          </tr>
        </thead>
        <tbody>
          {isLoading && <tr><td className="p-4" colSpan={100}><Spinner /></td></tr>}
          {showErr && <tr><td className="pt-2 pb-2 text-danger" colSpan={100}>{ errMsg }</td></tr>}
          {!isLoading && devices !== null && devices.length > 0 && devices.map(device => (<tr
            key={device.DeviceSerial}
            onClick={() => setCenterDev(device)}
          >
            <td>{ device.DeviceSerial }</td>
            <td>{ device.DeviceName || 'N/A' }</td>
            <td>{ device.DeviceTypeName || 'N/A' } ({ device.DeviceType || 'N/A' })</td>
            <td>{ device.DeviceState || 'UNKNOWN' }</td>
            <td>{ device.MaxCreatedAt.toLocaleString() }</td>
          </tr>))}
        </tbody>
      </Table>
    </Container>
  );
}
