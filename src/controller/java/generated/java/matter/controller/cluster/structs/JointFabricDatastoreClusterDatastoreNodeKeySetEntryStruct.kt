/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
package matter.controller.cluster.structs

import matter.controller.cluster.*
import matter.tlv.ContextSpecificTag
import matter.tlv.Tag
import matter.tlv.TlvReader
import matter.tlv.TlvWriter

class JointFabricDatastoreClusterDatastoreNodeKeySetEntryStruct(
  val nodeID: ULong,
  val groupKeySetID: UShort,
  val statusEntry: JointFabricDatastoreClusterDatastoreStatusEntryStruct,
) {
  override fun toString(): String = buildString {
    append("JointFabricDatastoreClusterDatastoreNodeKeySetEntryStruct {\n")
    append("\tnodeID : $nodeID\n")
    append("\tgroupKeySetID : $groupKeySetID\n")
    append("\tstatusEntry : $statusEntry\n")
    append("}\n")
  }

  fun toTlv(tlvTag: Tag, tlvWriter: TlvWriter) {
    tlvWriter.apply {
      startStructure(tlvTag)
      put(ContextSpecificTag(TAG_NODE_ID), nodeID)
      put(ContextSpecificTag(TAG_GROUP_KEY_SET_ID), groupKeySetID)
      statusEntry.toTlv(ContextSpecificTag(TAG_STATUS_ENTRY), this)
      endStructure()
    }
  }

  companion object {
    private const val TAG_NODE_ID = 0
    private const val TAG_GROUP_KEY_SET_ID = 1
    private const val TAG_STATUS_ENTRY = 2

    fun fromTlv(
      tlvTag: Tag,
      tlvReader: TlvReader,
    ): JointFabricDatastoreClusterDatastoreNodeKeySetEntryStruct {
      tlvReader.enterStructure(tlvTag)
      val nodeID = tlvReader.getULong(ContextSpecificTag(TAG_NODE_ID))
      val groupKeySetID = tlvReader.getUShort(ContextSpecificTag(TAG_GROUP_KEY_SET_ID))
      val statusEntry =
        JointFabricDatastoreClusterDatastoreStatusEntryStruct.fromTlv(
          ContextSpecificTag(TAG_STATUS_ENTRY),
          tlvReader,
        )

      tlvReader.exitContainer()

      return JointFabricDatastoreClusterDatastoreNodeKeySetEntryStruct(
        nodeID,
        groupKeySetID,
        statusEntry,
      )
    }
  }
}
