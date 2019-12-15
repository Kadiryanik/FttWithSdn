//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package src;

simple Controller
{
    parameters:
        volatile int id;
        @display("i=block/cogwheel");
    gates:
        inout gate[];
}

simple Switch
{
    parameters:
        volatile int defaultRoute;
        volatile int id;
        @display("i=block/switch");
    gates:
        inout gate[];
}

network TestNetwork
{
    @display("bgb=387,261");
    types:
        channel Channel extends ned.DelayChannel
        {
            delay = 20ms;
        }
    submodules:
        controller: Controller {
            @display("p=175,30");
        }
        // TODO: use array, how to set position of each switch when using array?
        switch_0: Switch {
            @display("p=75,60");
        }
        switch_1: Switch {
            @display("p=270,60");
        }
        switch_2: Switch {
            @display("p=75,135");
        }
        switch_3: Switch {
            @display("p=270,135");
        }
        switch_4: Switch {
            @display("p=75,205");
        }
    connections:
        // switch 0
        switch_0.gate++ <--> Channel <--> controller.gate++;
        // switch 1
        switch_1.gate++ <--> Channel <--> controller.gate++;
        // switch 2
        switch_2.gate++ <--> Channel <--> controller.gate++;
        switch_2.gate++ <--> Channel <--> switch_0.gate++;
        // switch 3
        switch_3.gate++ <--> Channel <--> controller.gate++;
        switch_3.gate++ <--> Channel <--> switch_1.gate++;
        switch_3.gate++ <--> Channel <--> switch_2.gate++;
        // switch 4
        switch_4.gate++ <--> Channel <--> controller.gate++;
        switch_4.gate++ <--> Channel <--> switch_2.gate++;
}
