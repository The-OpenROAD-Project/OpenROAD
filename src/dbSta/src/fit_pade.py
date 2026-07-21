"""A script to fit a bivariate Padé approximation for Ceff shielding factor K."""

import argparse
import sys
import numpy as np
import pandas as pd
from scipy import optimize


def main():
  # 1. Parse command line arguments
  parser = argparse.ArgumentParser(
      description=(
          "Fit a bivariate Padé approximation for Ceff shielding factor K."
      )
  )
  parser.add_argument(
      "--csv_path",
      default="third_party/open_road/src/dbSta/src/ceff_training_data.csv",
      help="Path to the training CSV file.",
  )
  parser.add_argument(
      "--output", help="Path to output the generated coefficients array."
  )
  args = parser.parse_args()

  try:
    df = pd.read_csv(args.csv_path)
  except FileNotFoundError:
    print(f"Error: Could not find dataset at {args.csv_path}")
    sys.exit(1)

  # Filter dataset to realistic operational domain:
  # x <= 10.0, y in [0.1, 0.9], z in [0.2, 5.0]
  df_filtered = df[
      (df["x_Rratio"] <= 10.0)
      & (df["y_Cratio"] >= 0.1)
      & (df["y_Cratio"] <= 0.9)
      & (df["z_Tratio"] >= 0.2)
      & (df["z_Tratio"] <= 5.0)
  ].copy()

  inputs_x = df_filtered["x_Rratio"].values
  inputs_y = df_filtered["y_Cratio"].values
  inputs_z = df_filtered["z_Tratio"].values

  features_matrix = np.vstack((inputs_x, inputs_y, inputs_z)).T  # shape (N, 3)
  y_target = df_filtered["k_shield"].values  # shape (N,)

  # 2. Define 2nd-order Bivariate Polynomial mapping for Padé parameters
  # P(y, z) = c0 + c1*y + c2*z + c3*y^2 + c4*y*z + c5*z^2
  def eval_poly(y, z, c):
    return (
        c[0]
        + c[1] * y
        + c[2] * z
        + c[3] * (y**2)
        + c[4] * y * z
        + c[5] * (z**2)
    )

  # 3. Define the 3D Padé rational function
  def pade_model(coords, *p):
    x_val, y_val, z_val = coords
    a1_c = p[0:6]
    b1_c = p[6:12]
    b2_c = p[12:18]

    a1 = eval_poly(y_val, z_val, a1_c)
    b1 = eval_poly(y_val, z_val, b1_c)
    b2 = eval_poly(y_val, z_val, b2_c)

    num = 1.0 + a1 * x_val
    den = 1.0 + b1 * x_val + b2 * (x_val**2)

    k_val = num / np.maximum(den, 1e-9)
    return np.clip(k_val, 0.0, 1.0)

  # 4. Fit the model on the full dataset
  train_coords = features_matrix.T
  p0 = np.ones(18) * 0.05

  popt, _ = optimize.curve_fit(
      pade_model, train_coords, y_target, p0=p0, maxfev=100000
  )

  # 5. Format coefficients
  a1_opt = popt[0:6]
  b1_opt = popt[6:12]
  b2_opt = popt[12:18]

  a1_str = ", ".join([f"{c:.8e}" for c in a1_opt])
  b1_str = ", ".join([f"{c:.8e}" for c in b1_opt])
  b2_str = ", ".join([f"{c:.8e}" for c in b2_opt])

  output_text = (
      f"    static constexpr double a1_coef[6] = {{ {a1_str} }};\n"
      f"    static constexpr double b1_coef[6] = {{ {b1_str} }};\n"
      f"    static constexpr double b2_coef[6] = {{ {b2_str} }};\n"
  )

  # 6. Output result
  if args.output:
    with open(args.output, "w") as f:
      f.write(output_text)
  else:
    print(output_text, end="")


if __name__ == "__main__":
  main()
