import './SideBar.less'
import React, { useState, useEffect, useMemo, useCallback } from 'react'
import TreeView from '@mui/lab/TreeView'
import TreeItem from '@mui/lab/TreeItem'
import { Paper, useTheme, alpha, Toolbar, Button } from '@mui/material'
import { FavoriteOutlined, SettingsInputHdmiOutlined, ExpandMore, ChevronRight } from '@mui/icons-material'

import type { TreeItemProps } from '@mui/lab/TreeItem'

// eslint-disable-next-line react/jsx-key
const icons = [<FavoriteOutlined />, <SettingsInputHdmiOutlined />]

type TreeNode = Record<string, boolean>

const DraggableTreeItem: React.FC<TreeItemProps> = props => (
  <TreeItem
    ref={props.draggable
      ? useCallback((elt: Element) => elt?.addEventListener('focusin', e => e.stopImmediatePropagation()), [])
      : props.ref}
    {...props}
  />
)

const Item: React.FC<{ tree: TreeNode, parent: string, type: number }> = ({ type, tree, parent }) => {
  const nodes = []
  const subTrees = useMemo<Record<string, TreeNode>>(() => ({ }), [])
  const [, update] = useState(0)
  for (const name in tree) {
    const cur = parent + '/' + name
    const [trueName, data] = name.split('#EIM#', 2)
    nodes.push((
      <DraggableTreeItem
        key={cur}
        nodeId={cur}
        label={trueName}
        draggable={!tree[name]}
        onDragStart={e => {
          e.dataTransfer.setData('application/json', JSON.stringify({ eim: true, type: 'loadPlugin', data: data }))
          e.stopPropagation()
        }}
        onClick={() => {
          if (subTrees[name]) return
          $client.getExplorerData(type, cur.replace(/^\//, '')).then(it => {
            subTrees[name] = it
            update(flag => flag + 1)
          })
        }}
      >
        {tree[name] && (subTrees[name] ? <Item tree={subTrees[name]} parent={cur} type={type} /> : <TreeItem nodeId={cur + '$$EMPTY'} />)}
      </DraggableTreeItem>
    ))
  }
  return (
    <>
      {nodes}
    </>
  )
}

const Explorer: React.FC<{ type: number }> = ({ type }) => {
  const [tree, setTree] = useState<TreeNode>({})
  useEffect(() => {
    $client.getExplorerData(type, '').then(setTree)
  }, [])
  return (
    <TreeView
      defaultCollapseIcon={<ExpandMore />}
      defaultExpandIcon={<ChevronRight />}
      sx={{ flexGrow: 1, overflow: 'auto', whiteSpace: 'nowrap' }}
    >
      <Item tree={tree} parent='' type={type} />
    </TreeView>
  )
}

const AppBar: React.FC = () => {
  const theme = useTheme()
  const [type, setType] = useState(1)
  return (
    <nav className='side-bar' style={{ zIndex: theme.zIndex.appBar - 1 }}>
      <Paper square elevation={3} sx={{ background: theme.palette.background.bright }}>
        <Toolbar />
        <ul className='types'>
          {icons.map((icon, i) => (
            <Button
              key={i}
              component='li'
              onClick={() => setType(i)}
              sx={{
                color: i === type ? '#fff' : theme.palette.background.brightest,
                borderRadius: 0,
                boxShadow: i === type ? 'inset 2px 0px ' + theme.palette.primary.main : undefined,
                backgroundColor: i === type ? alpha(theme.palette.primary.main, 0.2) : undefined,
                '&:hover': {
                  color: '#fff',
                  backgroundColor: i === type ? alpha(theme.palette.primary.main, 0.2) : 'inherit'
                }
              }}
            >
              {icon}
            </Button>
          ))}
        </ul>
      </Paper>
      <div className='explorer'>
        <Toolbar />
        <Explorer type={type} key={type} />
      </div>
    </nav>
  )
}

export default AppBar
