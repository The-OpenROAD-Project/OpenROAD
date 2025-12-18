import chisel3._
import java.io.File

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.scalatest.ChiselSim
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

class TinyAdder extends Module {
  val io = IO(new Bundle {
    val a = Input(UInt(8.W))
    val b = Input(UInt(8.W))
    val out = Output(UInt(8.W))
  })
  io.out := io.a + io.b
}

class TinyAdderTests extends AnyFreeSpec with Matchers with ChiselSim {
  "add" in {
    simulate(new TinyAdder()) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.a.poke(30.U)
      dut.io.b.poke(12.U)
      dut.clock.step()
      dut.io.out.expect(42.U)
    }
  }
}
